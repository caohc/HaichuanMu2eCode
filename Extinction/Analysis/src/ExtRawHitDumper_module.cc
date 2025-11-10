// Ntuple dumper for MCs.
//
// Andrei Gaponenko, 2013

#include <string>
#include <vector>
#include <limits>
#include <cmath>

#include "cetlib_except/exception.h"
#include "CLHEP/Vector/ThreeVector.h"

#include "TDirectory.h"
#include "TH1.h"
#include "TTree.h"

#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "fhiclcpp/types/Atom.h"
#include "fhiclcpp/types/OptionalSequence.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/Provenance.h"
#include "art_root_io/TFileService.h"

#include "Offline/GlobalConstantsService/inc/GlobalConstantsHandle.hh"
#include "Offline/GlobalConstantsService/inc/ParticleDataList.hh"
#include "Offline/Mu2eUtilities/inc/SimParticleGetTau.hh"
#include "Offline/GeometryService/inc/GeomHandle.hh"
#include "Offline/GeometryService/inc/DetectorSystem.hh"

#include "KinKal/General/ParticleState.hh"
#include "Offline/RecoDataProducts/inc/ExtMonFNALRawHit.hh"


namespace mu2e {

  //================================================================

  struct ExtRawHit {

          int RunID;
          int SubRunID;
          long long EventID;
          unsigned int planeId;
          unsigned int moduleId;
          unsigned int chipCol;
          unsigned int chipRow;
          unsigned int Col;
          unsigned int Row;
          int clock;
          int tot;

    ExtRawHit() :  RunID(-1), SubRunID(-1), EventID(-1),
                   planeId(-1), moduleId(-1),
                   chipCol(-1), chipRow(-1),
                   Col(-1), Row(-1),
                   clock(-1), tot(-1)
                {}

    ExtRawHit(int runID, int subrunID, long long eventID, const ExtMonFNALRawHit& hit) :
            RunID(runID), SubRunID(subrunID),EventID(eventID),
            planeId(hit.pixelId().chip().module().plane()), moduleId(hit.pixelId().chip().module().number()),
            chipCol(hit.pixelId().chip().chipCol()), chipRow(hit.pixelId().chip().chipRow()),
            Col(hit.pixelId().col()), Row(hit.pixelId().row()),
            clock(hit.clock()), tot(hit.tot())
                {}

  }; // struct ExtRawHit

  //================================================================
  class ExtRawHitDumper : public art::EDAnalyzer {
    struct Config {
      using Name=fhicl::Name;
      using Comment=fhicl::Comment;
      fhicl::Atom<std::string> hits     {Name("hitsInputTag"     ), Comment("MC collection")};
    };

    typedef art::EDAnalyzer::Table<Config> Parameters;

  protected:

    art::InputTag hitsInputTag_;
    TTree *nt_;
    ExtRawHit hit_;

    public:
    explicit ExtRawHitDumper(const Parameters& pset);
    virtual void beginJob();
    virtual void analyze(const art::Event& event);
  };

  //================================================================
  ExtRawHitDumper::ExtRawHitDumper(const Parameters& pset)
    : art::EDAnalyzer(pset)
      , hitsInputTag_(pset().hits())
      , nt_(0)
  {

  }

  //================================================================
  void ExtRawHitDumper::beginJob() {
    art::ServiceHandle<art::TFileService> tfs;
    static const char branchDesc[] = "RunID/I:SubRunID/I:EventID/L:planeId/i:moduleId/i:chipCol/i:chipRow/i:Col/i:Row/i:clock/I:tot/i";
    nt_ = tfs->make<TTree>( "nt", "ExtRawHits ntuple");
    nt_->Branch("hits", &hit_, branchDesc);
  }

  //================================================================

  void ExtRawHitDumper::analyze(const art::Event& event) {

    const auto& ih = event.getValidHandle<ExtMonFNALRawHitCollection>(hitsInputTag_);

    for(const auto& i : *ih) {

      //std::cout<<"run "<<event.run()<<", subrun "<<event.subRun()<<", event "<<event.event()
      //        <<", plane : "<<i->moduleId().plane()<<", module : "<<i->moduleId().number()
      //        <<", particle : "<<i->simParticle()->id().asUint()<<", eTot : "<<i->totalEnergyDeposit()<<", eIon : "<<i->ionizingEnergyDeposit()
      //        <<", start : ("<<i->localStartPosition().x()<<", "<<i->localStartPosition().y()<<", "<<i->localStartPosition().z()<<", t = "<<i->startTime()<<") "
      //        <<", end : ("<<i->localEndPosition().x()<<", "<<i->localEndPosition().y()<<", "<<i->localEndPosition().z()<<", t = "<<i->endTime()<<") "              
      //        <<std::endl;

      hit_ = ExtRawHit(event.run(), event.subRun(), event.event(), i);
      nt_->Fill();
    }
  }

  //================================================================

} // namespace mu2e

DEFINE_ART_MODULE(mu2e::ExtRawHitDumper)

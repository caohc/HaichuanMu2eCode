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
#include "Offline/MCDataProducts/inc/ExtMonFNALSimHit.hh"


namespace mu2e {

  //================================================================

  struct SimuParticle {

          int RunID;
          int SubRunID;
          long long EventID;

          int ParticlePID;
          float ParticleStartT;
          float ParticleEndT;
          float ParticleStartX;
          float ParticleStartY;
          float ParticleStartZ;
          float ParticleEndX;
          float ParticleEndY;
          float ParticleEndZ;
          float ParticleStartPx;
          float ParticleStartPy;
          float ParticleStartPz;

          int ParentPID;
          float ParentStartT;
          float ParentEndT;
          float ParentStartX;
          float ParentStartY;
          float ParentStartZ;
          float ParentEndX;
          float ParentEndY;
          float ParentEndZ;
          float ParentStartPx;
          float ParentStartPy;
          float ParentStartPz;
          float ParentEndPx;
          float ParentEndPy;
          float ParentEndPz;


    SimuParticle() :  RunID(-1), SubRunID(-1), EventID(-1)
          , ParticlePID(0), ParticleStartT(0), ParticleEndT(0)
          , ParticleStartX(0), ParticleStartY(0), ParticleStartZ(0)
          , ParticleEndX(0), ParticleEndY(0), ParticleEndZ(0)
          , ParticleStartPx(0), ParticleStartPy(0), ParticleStartPz(0)
          , ParentPID(0), ParentStartT(0), ParentEndT(0)
          , ParentStartX(0), ParentStartY(0), ParentStartZ(0)
          , ParentEndX(0), ParentEndY(0), ParentEndZ(0)
          , ParentStartPx(0), ParentStartPy(0), ParentStartPz(0)
          , ParentEndPx(0), ParentEndPy(0), ParentEndPz(0)
          {}

    SimuParticle(int runID, int subrunID, long long eventID, const SimParticle& particle) :
            RunID(runID), SubRunID(subrunID),EventID(eventID)
          , ParticlePID(particle.pdgId()), ParticleStartT(particle.startGlobalTime()), ParticleEndT(particle.endGlobalTime())
          , ParticleStartX(particle.startPosition().x()), ParticleStartY(particle.startPosition().y()), ParticleStartZ(particle.startPosition().z())
          , ParticleEndX(particle.endPosition().x()), ParticleEndY(particle.endPosition().y()), ParticleEndZ(particle.endPosition().z())
          , ParticleStartPx(particle.startMomentum().x()), ParticleStartPy(particle.startMomentum().y()), ParticleStartPz(particle.startMomentum().z())
          , ParentPID(particle.parent()->pdgId()), ParentStartT(particle.parent()->startGlobalTime()), ParentEndT(particle.parent()->endGlobalTime())
          , ParentStartX(particle.parent()->startPosition().x()), ParentStartY(particle.parent()->startPosition().y()), ParentStartZ(particle.parent()->startPosition().z())
          , ParentEndX(particle.parent()->endPosition().x()), ParentEndY(particle.parent()->endPosition().y()), ParentEndZ(particle.parent()->endPosition().z())
          , ParentStartPx(particle.parent()->startMomentum().x()), ParentStartPy(particle.parent()->startMomentum().y()), ParentStartPz(particle.parent()->startMomentum().z())
          , ParentEndPx(particle.parent()->endMomentum().x()), ParentEndPy(particle.parent()->endMomentum().y()), ParentEndPz(particle.parent()->endMomentum().z())
          {}

  }; // struct SimuParticle

  //================================================================
  class mySimParticlesExtracter : public art::EDAnalyzer {
    struct Config {
      using Name=fhicl::Name;
      using Comment=fhicl::Comment;
      fhicl::Atom<std::string> hits     {Name("hitsInputTag"     ), Comment("MC collection")};
    };

    typedef art::EDAnalyzer::Table<Config> Parameters;

  protected:

    art::InputTag hitsInputTag_;
    TTree *nt_;
    SimuParticle hit_;

    public:
    explicit mySimParticlesExtracter(const Parameters& pset);
    virtual void beginJob();
    virtual void analyze(const art::Event& event);
  };

  //================================================================
  mySimParticlesExtracter::mySimParticlesExtracter(const Parameters& pset)
    : art::EDAnalyzer(pset)
      , hitsInputTag_(pset().hits())
      , nt_(0)
  {

  }

  //================================================================
  void mySimParticlesExtracter::beginJob() {

    art::ServiceHandle<art::TFileService> tfs;
    static const char branchDesc[] = "RunID/I:SubRunID/I:EventID/L:ParticlePID/I:ParticleStartT/F:ParticleEndT/F:ParticleStartX/F:ParticleStartY/F:ParticleStartZ/F:ParticleEndX/F:ParticleEndY/F:ParticleEndZ/F:ParticleStartPx/F:ParticleStartPy/F:ParticleStartPz/F:ParentPID/I:ParentStartT/F:ParentEndT/F:ParentStartX/F:ParentStartY/F:ParentStartZ/F:ParentEndX/F:ParentEndY/F:ParentEndZ/F:ParentStartPx/F:ParentStartPy/F:ParentStartPz/F:ParentEndPx/F:ParentEndPy/F:ParentEndPz/F";

    nt_ = tfs->make<TTree>( "nt", "SimuParticles ntuple");
    nt_->Branch("hits", &hit_, branchDesc);
  }

  //================================================================

  void mySimParticlesExtracter::analyze(const art::Event& event) {

    const auto& ih = event.getValidHandle<SimParticleCollection>(hitsInputTag_);

    art::Ptr<SimParticle> Parent;

    for(const auto& i : *ih) 
   {
         const SimParticle& particle = i.second;

        if(particle.hasParent())
       {	   
           Parent= particle.parent();
           //std::cout<<"Parent : "<<Parent->pdgId()<<", ("<<
           //Parent->startPosition().x()<<", "<<Parent->startPosition().y()<<", "<<Parent->startPosition().z()<<") " << Parent->startGlobalTime() <<
           //std::endl; 

           hit_ = SimuParticle(event.run(), event.subRun(), event.event(), particle);

           //std::cout<<"Parent : "<<hit_.ParentPID<<", ("<<
           //hit_.ParentStartX<<", "<<hit_.ParentStartY<<", "<<hit_.ParentStartZ<<") " << hit_.ParentStartT<<
           //std::endl;

           nt_->Fill();
       } 

   }
 }

  //================================================================

} // namespace mu2e

DEFINE_ART_MODULE(mu2e::mySimParticlesExtracter)

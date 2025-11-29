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
#include "TFile.h"

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

          int ParentPID;
          int ParticlePID;
          unsigned int StartVolumeID;
	  unsigned int EndVolumeID; 

    SimuParticle() :
	    ParentPID(0), ParticlePID(0), StartVolumeID(-1), EndVolumeID(-1)
          {}

    SimuParticle(int parent, int particle, unsigned int start, unsigned int end) :
            ParentPID(parent), ParticlePID(particle), StartVolumeID(start), EndVolumeID(end)
          {}

  }; // struct SimuParticle

  //================================================================
  class mySimVolumeIDExtracter : public art::EDAnalyzer {
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
    explicit mySimVolumeIDExtracter(const Parameters& pset);
    virtual void beginJob();
    virtual void endJob();
    virtual void analyze(const art::Event& event);
  };

  //================================================================
  mySimVolumeIDExtracter::mySimVolumeIDExtracter(const Parameters& pset)
    : art::EDAnalyzer(pset)
      , hitsInputTag_(pset().hits())
      , nt_(0)
  {
  }

  //================================================================
  void mySimVolumeIDExtracter::beginJob() {
    art::ServiceHandle<art::TFileService> tfs;
    static const char branchDesc[] = "ParentPID/I:ParticlePID/I:StartVolumeID/i:EndVolumeID/i";

    nt_ = tfs->make<TTree>( "nt", "SimuParticles ntuple");
    nt_->Branch("hits", &hit_, branchDesc);

  }

  //================================================================

  void mySimVolumeIDExtracter::analyze(const art::Event& event) {

    const auto& ih = event.getValidHandle<SimParticleCollection>(hitsInputTag_);

    art::Ptr<SimParticle> Parent;

    for(const auto& i : *ih) 
   {
         const SimParticle& particle = i.second;

        if(!particle.hasParent()) hit_ = SimuParticle(0, particle.pdgId(), particle.startVolumeIndex(), particle.endVolumeIndex() ) ;
	else hit_ = SimuParticle(particle.parent()->pdgId(), particle.pdgId(), particle.startVolumeIndex(), particle.endVolumeIndex() );

           //std::cout<<"Parent : "<<hit_.ParentPID<<", ("<<
           //hit_.ParentStartX<<", "<<hit_.ParentStartY<<", "<<hit_.ParentStartZ<<") " << hit_.ParentStartT<<
           //std::endl;

        nt_->Fill();

   }
 }

  //================================================================

void mySimVolumeIDExtracter::endJob() {

   if (nt_) {
    TFile* f = nt_->GetCurrentFile();
    if (f) {
      f->cd();
      nt_->Write("", TObject::kOverwrite);
      f->Write();
    }
  }
}



} // namespace mu2e

DEFINE_ART_MODULE(mu2e::mySimVolumeIDExtracter)

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
          float ParticleStartX;
          float ParticleStartY;
          float ParticleStartZ;
          float ParticleEndX;
          float ParticleEndY;
          float ParticleEndZ;

    SimuParticle() :
	    ParentPID(0), ParticlePID(0), StartVolumeID(-1), EndVolumeID(-1)
          , ParticleStartX(0), ParticleStartY(0), ParticleStartZ(0)
          , ParticleEndX(0), ParticleEndY(0), ParticleEndZ(0)
	  {}

    SimuParticle(int parent, const SimParticle& particle) :
            ParentPID(parent), ParticlePID(particle.pdgId())
	  , StartVolumeID(particle.startVolumeIndex()), EndVolumeID(particle.endVolumeIndex())
          , ParticleStartX(particle.startPosition().x()), ParticleStartY(particle.startPosition().y()), ParticleStartZ(particle.startPosition().z())
          , ParticleEndX(particle.endPosition().x()), ParticleEndY(particle.endPosition().y()), ParticleEndZ(particle.endPosition().z())
	  {}

  }; // struct SimuParticle

  //================================================================
  class mySimPositionIDExtracter : public art::EDAnalyzer {
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
    explicit mySimPositionIDExtracter(const Parameters& pset);
    virtual void beginJob();
    virtual void endJob();
    virtual void analyze(const art::Event& event);
  };

  //================================================================
  mySimPositionIDExtracter::mySimPositionIDExtracter(const Parameters& pset)
    : art::EDAnalyzer(pset)
      , hitsInputTag_(pset().hits())
      , nt_(0)
  {
  }

  //================================================================
  void mySimPositionIDExtracter::beginJob() {
    art::ServiceHandle<art::TFileService> tfs;
    static const char branchDesc[] = "ParentPID/I:ParticlePID/I:StartVolumeID/i:EndVolumeID/i:ParticleStartX/F:ParticleStartY/F:ParticleStartZ/F:ParticleEndX/F:ParticleEndY/F:ParticleEndZ/F";

    nt_ = tfs->make<TTree>( "nt", "SimuParticles ntuple");
    nt_->Branch("hits", &hit_, branchDesc);

  }

  //================================================================

  void mySimPositionIDExtracter::analyze(const art::Event& event) {

    const auto& ih = event.getValidHandle<SimParticleCollection>(hitsInputTag_);

    art::Ptr<SimParticle> Parent;

    float X, Y, Z;
    for(const auto& i : *ih) 
   {
       const SimParticle& particle = i.second;

       X=particle.startPosition().x();
       Y=particle.startPosition().y();
       Z=particle.startPosition().z();

       if(Z>-6300 && Z<-6000 && X<3950 && X>3850 && Y>-20 && Y<20)
      {
       	  if(!particle.hasParent()) hit_ = SimuParticle(0, particle) ;
    	  else hit_ = SimuParticle(particle.parent()->pdgId(), particle);
          nt_->Fill();
       }
   }
 }

  //================================================================

void mySimPositionIDExtracter::endJob() {

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

DEFINE_ART_MODULE(mu2e::mySimPositionIDExtracter)

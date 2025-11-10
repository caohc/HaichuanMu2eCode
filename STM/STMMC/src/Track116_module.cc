// Ntuple dumper for StepPointMCs.
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

#include "Offline/MCDataProducts/inc/StepPointMC.hh"

#include "Offline/GlobalConstantsService/inc/GlobalConstantsHandle.hh"
#include "Offline/GlobalConstantsService/inc/ParticleDataList.hh"
#include "Offline/Mu2eUtilities/inc/SimParticleGetTau.hh"
#include "Offline/GeometryService/inc/GeomHandle.hh"
#include "Offline/GeometryService/inc/DetectorSystem.hh"
#include "Offline/GlobalConstantsService/inc/ParticleDataList.hh"
#include "Offline/MCDataProducts/inc/MCRelationship.hh"

#include "KinKal/General/ParticleState.hh"

namespace mu2e {

  //================================================================
  double getCharge(PDGCode::type pdgId) {
    // unlike generic conditions, MC particle data
    // should not change run-to-run, so static is safe
    // use static for efficiency
    static GlobalConstantsHandle<ParticleDataList> pdt;

    return pdt->particle(pdgId).charge();
  }

  //================================================================
  double getKineticEnergy(const StepPointMC& hit) {
    // unlike generic conditions, MC particle data
    // should not change run-to-run, so static is safe
    // use static for efficiency
    static GlobalConstantsHandle<ParticleDataList> pdt;

    const double mass = pdt->particle(hit.simParticle()->pdgId()).mass();
    return sqrt(hit.momentum().mag2() + std::pow(mass, 2)) - mass;
  }

  //================================================================
  struct VDHit {
    
    long track_key;
    long parent_key;  
    float x;
    float y;
    float z;
    float time;

    float px;
    float py;
    float pz;
    float pmag;
    float ek;

    float charge;
    int   pdgId;
    int   parentId;
    unsigned particleId;
    unsigned volumeCopyNumber;

    VDHit() : track_key(-1), parent_key(-1)
	      , x(std::numeric_limits<double>::max())
              , y(std::numeric_limits<double>::max())
              , z(std::numeric_limits<double>::max())
              , time(std::numeric_limits<double>::max())
                , px(std::numeric_limits<double>::max())
                , py(std::numeric_limits<double>::max())
                , pz(std::numeric_limits<double>::max())
                , pmag(std::numeric_limits<double>::max())
                , ek(std::numeric_limits<double>::max())
                , charge(std::numeric_limits<double>::max())
                , pdgId(0)
	        , parentId(0)
                , particleId(-1U)
                , volumeCopyNumber(-1U)
                {}

    //----------------------------------------------------------------
    VDHit( const StepPointMC& hit)
      :   x(hit.position().x())
        , y(hit.position().y())
        , z(hit.position().z())
        , time(hit.time())
        , px(hit.momentum().x())
        , py(hit.momentum().y())
        , pz(hit.momentum().z())
        , pmag(hit.momentum().mag())
          , ek(getKineticEnergy(hit))
          , charge(getCharge(hit.simParticle()->pdgId()))
          , pdgId(hit.simParticle()->pdgId())
	  , parentId(0)
          , particleId(hit.simParticle()->id().asUint())
          , volumeCopyNumber(hit.volumeId())
          {
	     track_key = hit.simParticle().key();
             parent_key =hit.simParticle()->parent().key();
	  }

    VDHit( const StepPointMC& hit, int parent)
      :   x(hit.position().x())
        , y(hit.position().y())
        , z(hit.position().z())
        , time(hit.time())
        , px(hit.momentum().x())
        , py(hit.momentum().y())
        , pz(hit.momentum().z())
        , pmag(hit.momentum().mag())
          , ek(getKineticEnergy(hit))
          , charge(getCharge(hit.simParticle()->pdgId()))
          , pdgId(hit.simParticle()->pdgId())
          , parentId(parent)
          , particleId(hit.simParticle()->id().asUint())
          , volumeCopyNumber(hit.volumeId())
          {
             track_key = hit.simParticle().key();
             parent_key =hit.simParticle()->parent().key();
          }

  }; // struct VDHit

  //================================================================
  class StepPointMCDumper : public art::EDAnalyzer {
    struct Config {
      using Name=fhicl::Name;
      using Comment=fhicl::Comment;
      fhicl::Atom<std::string> hits     {Name("hitsInputTag"     ), Comment("StepPointMC collection")};
    };
    typedef art::EDAnalyzer::Table<Config> Parameters;

    art::InputTag hitsInputTag_;


    // Members needed to write the ntuple
    TTree *nt_;
    VDHit hit_;

    public:
    explicit StepPointMCDumper(const Parameters& pset);
    virtual void beginJob();
    virtual void analyze(const art::Event& event);
  };

  //================================================================
  StepPointMCDumper::StepPointMCDumper(const Parameters& pset)
    : art::EDAnalyzer(pset)
      , hitsInputTag_(pset().hits())
      , nt_(0)
  {
  }

  //================================================================
  void StepPointMCDumper::beginJob() {
    art::ServiceHandle<art::TFileService> tfs;
    static const char branchDesc[] = "track_key/L:parent_key/L:x/F:y/F:z/F:time/F:px/F:py/F:pz/F:pmag/F:ek/F:charge/F:pdgId/I:parentId/I:particleId/i:volumeCopy/i";
    nt_ = tfs->make<TTree>( "nt", "StepPointMCDumper ntuple");
    nt_->Branch("hits", &hit_, branchDesc);
  }

  //================================================================
  void StepPointMCDumper::analyze(const art::Event& event) {

    const art::ValidHandle<std::vector<mu2e::StepPointMC> >& ih = event.getValidHandle<StepPointMCCollection>(hitsInputTag_);
    const art::ValidHandle<std::vector<mu2e::StepPointMC> >& jh = event.getValidHandle<StepPointMCCollection>(hitsInputTag_);


      for(const StepPointMC& i : *ih)
     {
        if(i.volumeId()==116)
       { 	

           for(const StepPointMC& j : *jh)
          {    	       
	       
	       if(j.volumeId()==10)
	      {
		
	         const art::Ptr<SimParticle> vd116_particle = j.simParticle();
                 const art::Ptr<SimParticle> vd10_particle  = i.simParticle();

	         MCRelationship relation(vd116_particle, vd10_particle);
                 if (relation.relationship() == MCRelationship::same || relation.relationship() == MCRelationship::mother || relation.relationship() == MCRelationship::umother)
		{ 
	           hit_ = VDHit(i, j.simParticle()->pdgId());
                   nt_->Fill();
		}
              }
          }

       }
     }
  } // analyze(event)

  //================================================================

} // namespace mu2e

DEFINE_ART_MODULE(mu2e::StepPointMCDumper)

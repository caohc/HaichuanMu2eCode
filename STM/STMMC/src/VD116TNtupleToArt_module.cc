#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Persistency/Provenance/ProductID.h"
#include "cetlib_except/exception.h"
#include "fhiclcpp/types/Atom.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "CLHEP/Vector/LorentzVector.h"
#include "CLHEP/Vector/ThreeVector.h"

#include "Offline/GlobalConstantsService/inc/GlobalConstantsHandle.hh"
#include "Offline/GlobalConstantsService/inc/ParticleDataList.hh"
#include "Offline/MCDataProducts/inc/GenId.hh"
#include "Offline/MCDataProducts/inc/GenParticle.hh"
#include "Offline/MCDataProducts/inc/PhysicalVolumeInfo.hh"
#include "Offline/MCDataProducts/inc/PhysicalVolumeInfoMultiCollection.hh"
#include "Offline/MCDataProducts/inc/ProcessCode.hh"
#include "Offline/MCDataProducts/inc/SimParticle.hh"
#include "Offline/MCDataProducts/inc/StepPointMC.hh"

#include "TFile.h"
#include "TNtuple.h"

namespace mu2e {

  namespace {
    struct HitRow {
      Float_t x = 0.0f;
      Float_t y = 0.0f;
      Float_t z = 0.0f;
      Float_t time = 0.0f;
      Float_t px = 0.0f;
      Float_t py = 0.0f;
      Float_t pz = 0.0f;
      Float_t pmag = 0.0f;
      Float_t ek = 0.0f;
      Float_t charge = 0.0f;
      Float_t pdgId = 0.0f;
      Float_t particleId = 0.0f;
      Float_t volumeCopy = 0.0f;
    };
  }

  class VD116TNtupleToArt : public art::EDProducer {
    public:
      struct Config {
        using Name = fhicl::Name;
        using Comment = fhicl::Comment;

        fhicl::Atom<std::string> rootFile{
          Name("rootFile"),
          Comment("Input ROOT file containing the TNtuple"),
          "Source/Gamma.root"
        };

        fhicl::Atom<std::string> treeName{
          Name("treeName"),
          Comment("Input TNtuple/TTree name"),
          "nt"
        };

        fhicl::Atom<unsigned> hitsPerEvent{
          Name("hitsPerEvent"),
          Comment("Number of sequential TNtuple rows packed into one art event"),
          1u
        };

        fhicl::Atom<int> maxRows{
          Name("maxRows"),
          Comment("Maximum number of TNtuple rows to read; negative means all"),
          -1
        };

        fhicl::Atom<bool> verbose{
          Name("verbose"),
          Comment("Print summary information"),
          false
        };
      };

      using Parameters = art::EDProducer::Table<Config>;

      explicit VD116TNtupleToArt(const Parameters& conf);
      void produce(art::Event& event) override;

    private:
      void loadHits(const std::string& rootFile, const std::string& treeName, int maxRows);

      std::vector<HitRow> hits_;
      std::size_t nextHit_{0};
      unsigned hitsPerEvent_{1};
      bool verbose_{false};
      bool warnedExhausted_{false};
      GlobalConstantsHandle<ParticleDataList> pdt_;
    };

  VD116TNtupleToArt::VD116TNtupleToArt(const Parameters& conf)
    : art::EDProducer{conf}
    , hitsPerEvent_{conf().hitsPerEvent()}
    , verbose_{conf().verbose()}
  {
    produces<GenParticleCollection>();
    produces<SimParticleCollection>();
    produces<StepPointMCCollection>();

    loadHits(conf().rootFile(), conf().treeName(), conf().maxRows());

    if (verbose_) {
      mf::LogInfo("VD116TNtupleToArt")
        << "Loaded " << hits_.size() << " hits from " << conf().rootFile()
        << ", hitsPerEvent = " << hitsPerEvent_;
    }
  }

  void VD116TNtupleToArt::loadHits(const std::string& rootFile,
                                   const std::string& treeName,
                                   int maxRows) {
    std::unique_ptr<TFile> inputFile(TFile::Open(rootFile.c_str(), "READ"));
    if (!inputFile || inputFile->IsZombie()) {
      throw cet::exception("VD116TNtupleToArt")
        << "Cannot open input ROOT file: " << rootFile << "\n";
    }

    TNtuple* nt = dynamic_cast<TNtuple*>(inputFile->Get(treeName.c_str()));
    if (!nt) {
      throw cet::exception("VD116TNtupleToArt")
        << "Cannot find TNtuple '" << treeName << "' in file " << rootFile << "\n";
    }

    HitRow row;
    nt->SetBranchAddress("x", &row.x);
    nt->SetBranchAddress("y", &row.y);
    nt->SetBranchAddress("z", &row.z);
    nt->SetBranchAddress("time", &row.time);
    nt->SetBranchAddress("px", &row.px);
    nt->SetBranchAddress("py", &row.py);
    nt->SetBranchAddress("pz", &row.pz);
    nt->SetBranchAddress("pmag", &row.pmag);
    nt->SetBranchAddress("ek", &row.ek);
    nt->SetBranchAddress("charge", &row.charge);
    nt->SetBranchAddress("pdgId", &row.pdgId);
    nt->SetBranchAddress("particleId", &row.particleId);
    nt->SetBranchAddress("volumeCopy", &row.volumeCopy);

    const Long64_t nEntries = nt->GetEntries();
    const Long64_t rowsToRead =
      (maxRows >= 0) ? std::min<Long64_t>(nEntries, maxRows) : nEntries;

    hits_.reserve(static_cast<std::size_t>(rowsToRead));
    for (Long64_t i = 0; i < rowsToRead; ++i) {
      nt->GetEntry(i);
      hits_.push_back(row);
    }
  }

  void VD116TNtupleToArt::produce(art::Event& event) {
    auto genParticles = std::make_unique<GenParticleCollection>();
    auto simParticles = std::make_unique<SimParticleCollection>();
    auto stepPoints = std::make_unique<StepPointMCCollection>();

    const art::ProductID genPID = event.getProductID<GenParticleCollection>();
    const art::EDProductGetter* genGetter = event.productGetter(genPID);
    const art::ProductID simPID = event.getProductID<SimParticleCollection>();
    const art::EDProductGetter* simGetter = event.productGetter(simPID);

    if (nextHit_ >= hits_.size()) {
      if (!warnedExhausted_) {
        warnedExhausted_ = true;
        mf::LogWarning("VD116TNtupleToArt")
          << "Input TNtuple is exhausted. Remaining art events will be empty.";
      }

      event.put(std::move(genParticles));
      event.put(std::move(simParticles));
      event.put(std::move(stepPoints));
      return;
    }

    const std::size_t remaining = hits_.size() - nextHit_;
    const std::size_t nThisEvent = std::min<std::size_t>(hitsPerEvent_, remaining);

    genParticles->reserve(nThisEvent);
    stepPoints->reserve(nThisEvent);

    for (std::size_t i = 0; i < nThisEvent; ++i, ++nextHit_) {
      const HitRow& hit = hits_[nextHit_];

      const int pdgId = static_cast<int>(std::lround(hit.pdgId));
      const unsigned volumeId = static_cast<unsigned>(std::lround(hit.volumeCopy));
      const double mass = pdt_->particle(static_cast<PDGCode::type>(pdgId)).mass();
      const double momentumMag2 =
        static_cast<double>(hit.px) * hit.px +
        static_cast<double>(hit.py) * hit.py +
        static_cast<double>(hit.pz) * hit.pz;
      const double energy =
        (mass == 0.0) ? static_cast<double>(hit.pmag)
                      : std::sqrt(std::max(0.0, momentumMag2 + mass * mass));

      const CLHEP::Hep3Vector position(hit.x, hit.y, hit.z);
      const CLHEP::HepLorentzVector momentum(hit.px, hit.py, hit.pz, energy);
      const art::Ptr<GenParticle> genPtr(
        genPID,
        genParticles->size(),
        genGetter
      );

      genParticles->emplace_back(
        static_cast<PDGCode::type>(pdgId),
        GenId::fromStepPointMCs,
        position,
        momentum,
        hit.time,
        0.0
      );

      const cet::map_vector_key simKey(static_cast<unsigned>(i + 1));
      (*simParticles)[simKey] = SimParticle(
        simKey,
        1u,
        art::Ptr<SimParticle>(),
        static_cast<PDGCode::type>(pdgId),
        genPtr,
        position,
        momentum,
        hit.time,
        0.0,
        volumeId,
        0u,
        ProcessCode::mu2ePrimary
      );

      SimParticle& sim = (*simParticles)[simKey];
      sim.addEndInfo(
        position,
        momentum,
        hit.time,
        0.0,
        volumeId,
        0u,
        ProcessCode::Transportation,
        hit.ek,
        1,
        0.0
      );

      const art::Ptr<SimParticle> simPtr(simPID, simKey.asUint(), simGetter);
      const CLHEP::Hep3Vector stepMomentum(hit.px, hit.py, hit.pz);

      stepPoints->emplace_back(
        simPtr,
        volumeId,
        0.0,
        0.0,
        0.0,
        hit.time,
        0.0,
        position,
        position,
        stepMomentum,
        stepMomentum,
        0.0,
        ProcessCode::Transportation
      );
    }

    event.put(std::move(genParticles));
    event.put(std::move(simParticles));
    event.put(std::move(stepPoints));
  }

}  // namespace mu2e

DEFINE_ART_MODULE(mu2e::VD116TNtupleToArt)

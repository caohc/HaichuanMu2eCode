#include <memory>
#include <string>

#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/SubRun.h"
#include "fhiclcpp/types/Atom.h"

#include "Offline/MCDataProducts/inc/PhysicalVolumeInfo.hh"
#include "Offline/MCDataProducts/inc/PhysicalVolumeInfoMultiCollection.hh"

namespace mu2e {

  class VD116VolumeInfoProducer : public art::EDProducer {
    public:
      struct Config {
        using Name = fhicl::Name;
        using Comment = fhicl::Comment;

        fhicl::Atom<unsigned> volumeId{
          Name("volumeId"),
          Comment("Volume identifier stored in the physical-volume map"),
          116u
        };

        fhicl::Atom<std::string> volumeName{
          Name("volumeName"),
          Comment("Volume name stored in the physical-volume map"),
          "VirtualDetector"
        };

        fhicl::Atom<std::string> materialName{
          Name("materialName"),
          Comment("Material name stored in the physical-volume map"),
          "G4_Galactic"
        };
      };

      using Parameters = art::EDProducer::Table<Config>;

      explicit VD116VolumeInfoProducer(const Parameters& conf);
      void produce(art::Event& event) override;
      void endSubRun(art::SubRun& sr) override;

    private:
      unsigned volumeId_{116u};
      std::string volumeName_;
      std::string materialName_;
    };

  VD116VolumeInfoProducer::VD116VolumeInfoProducer(const Parameters& conf)
    : art::EDProducer{conf}
    , volumeId_{conf().volumeId()}
    , volumeName_{conf().volumeName()}
    , materialName_{conf().materialName()}
  {
    produces<PhysicalVolumeInfoMultiCollection, art::InSubRun>();
  }

  void VD116VolumeInfoProducer::produce(art::Event&) {}

  void VD116VolumeInfoProducer::endSubRun(art::SubRun& sr) {
    auto out = std::make_unique<PhysicalVolumeInfoMultiCollection>();
    out->resize(2);

    // Stage 0 mimics the upstream resampler stage.
    (*out)[0][cet::map_vector_key(volumeId_)] =
      PhysicalVolumeInfo(volumeName_, volumeId_, materialName_);

    // Stage 1 mimics BeamToVD116 g4run output that is consumed by stmResampler.
    (*out)[1][cet::map_vector_key(volumeId_)] =
      PhysicalVolumeInfo(volumeName_, volumeId_, materialName_);
    sr.put(std::move(out), art::fullSubRun());
  }

}  // namespace mu2e

DEFINE_ART_MODULE(mu2e::VD116VolumeInfoProducer)

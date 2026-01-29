// STMDepositEnergy_module.cc
// 计算探测器能量沉积并保存到指定 ROOT 文件（仅 histogram，无 TTree）

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Run.h"

#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"

#include "Offline/MCDataProducts/inc/StepPointMC.hh"

#include "TFile.h"
#include "TH1F.h"

#include <iostream>
#include <string>
#include <map>
#include <memory>

namespace mu2e {

  class STMDepositEnergy : public art::EDAnalyzer {
  public:
    struct Config {
      fhicl::Atom<std::string> stepPointMCTag {
        fhicl::Name("stepPointMCTag"),
        fhicl::Comment("Tag for StepPointMC collection")
      };
      fhicl::Atom<int> verboseLevel {
        fhicl::Name("verboseLevel"),
        fhicl::Comment("Verbose level"), 0
      };
      fhicl::Atom<bool> groupByVolume {
        fhicl::Name("groupByVolume"),
        fhicl::Comment("Group energy deposits by volume ID"), false
      };
      fhicl::Atom<std::string> outputFileName {
        fhicl::Name("outputFileName"),
        fhicl::Comment("Output ROOT file name")
      };
    };

    using Parameters = art::EDAnalyzer::Table<Config>;
    explicit STMDepositEnergy(const Parameters& conf);

    void beginJob() override;
    void analyze(const art::Event& event) override;
    void endJob() override;

  private:
    art::InputTag _stepPointMCTag;
    int _verboseLevel;
    bool _groupByVolume;
    std::string _outputFileName;

    struct EnergyDeposits {
      double total = 0.0;
      double visible = 0.0;
      double nonIonizing = 0.0;
    };

    EnergyDeposits _totalEnergySum;
    int _totalEvents = 0;

    // ROOT
    std::unique_ptr<TFile> _outputFile;
    TH1F* _hTotalEnergy = nullptr;
    TH1F* _hVisibleEnergy = nullptr;
    TH1F* _hNonIonizingEnergy = nullptr;
  };

  // ------------------------------------------------------------------

  STMDepositEnergy::STMDepositEnergy(const Parameters& conf)
    : art::EDAnalyzer(conf),
      _stepPointMCTag(conf().stepPointMCTag()),
      _verboseLevel(conf().verboseLevel()),
      _groupByVolume(conf().groupByVolume()),
      _outputFileName(conf().outputFileName())
  {
    std::cout << "STMDepositEnergy initialized with tag: "
              << _stepPointMCTag << std::endl;
    std::cout << "Output file: " << _outputFileName << std::endl;
  }

  // ------------------------------------------------------------------

  void STMDepositEnergy::beginJob() {
    _outputFile = std::make_unique<TFile>(_outputFileName.c_str(), "RECREATE");

    // bin 数扩大 10 倍，range 不变
    _hTotalEnergy =
      new TH1F("hTotalEnergy",
               "Total Energy Deposit;Energy (MeV);Events",
               1000, 0, 100);

    _hVisibleEnergy =
      new TH1F("hVisibleEnergy",
               "Visible Energy Deposit;Energy (MeV);Events",
               1000, 0, 100);

    _hNonIonizingEnergy =
      new TH1F("hNonIonizingEnergy",
               "Non-Ionizing Energy Deposit;Energy (MeV);Events",
               1000, 0, 20);
  }

  // ------------------------------------------------------------------

  void STMDepositEnergy::analyze(const art::Event& event) {
    auto stepsHandle =
      event.getValidHandle<StepPointMCCollection>(_stepPointMCTag);
    const auto& steps = *stepsHandle;

    EnergyDeposits energyDeposits;
    std::map<int, EnergyDeposits> volumeEnergyMap;

    for (const auto& step : steps) {
      energyDeposits.total       += step.totalEDep();
      energyDeposits.visible     += step.visibleEDep();
      energyDeposits.nonIonizing += step.nonIonizingEDep();

      if (_groupByVolume) {
        int volumeId = step.volumeId();
        volumeEnergyMap[volumeId].total       += step.totalEDep();
        volumeEnergyMap[volumeId].visible     += step.visibleEDep();
        volumeEnergyMap[volumeId].nonIonizing += step.nonIonizingEDep();
      }
    }

    _totalEnergySum.total       += energyDeposits.total;
    _totalEnergySum.visible     += energyDeposits.visible;
    _totalEnergySum.nonIonizing += energyDeposits.nonIonizing;
    _totalEvents++;

    // 仅当 total > 1e-8 时才填充 histogram
    if (energyDeposits.total > 1e-8) {
      _hTotalEnergy->Fill(energyDeposits.total);
      _hVisibleEnergy->Fill(energyDeposits.visible);
      _hNonIonizingEnergy->Fill(energyDeposits.nonIonizing);
    }

    if (_verboseLevel > 0) {
      std::cout << "Event " << event.id().event()
                << " | Total=" << energyDeposits.total
                << " Visible=" << energyDeposits.visible
                << " NonIonizing=" << energyDeposits.nonIonizing
                << std::endl;
    }
  }

  // ------------------------------------------------------------------

  void STMDepositEnergy::endJob() {
    if (_totalEvents > 0) {
      std::cout << "\n===== Summary (" << _outputFileName << ") =====\n"
                << "Total events processed: " << _totalEvents << "\n"
                << "Average total energy deposit: "
                << _totalEnergySum.total / _totalEvents << " MeV\n"
                << "Average visible energy deposit: "
                << _totalEnergySum.visible / _totalEvents << " MeV\n"
                << "Average non-ionizing energy deposit: "
                << _totalEnergySum.nonIonizing / _totalEvents << " MeV\n"
                << "=============================================\n";
    }

    if (_outputFile) {
      _outputFile->cd();
      _hTotalEnergy->Write();
      _hVisibleEnergy->Write();
      _hNonIonizingEnergy->Write();
      _outputFile->Close();
    }
  }

} // namespace mu2e

DEFINE_ART_MODULE(mu2e::STMDepositEnergy)

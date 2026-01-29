// STMDepositEnergy_module.cc

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"

#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "Offline/MCDataProducts/inc/StepPointMC.hh"


#include "art_root_io/TFileService.h"
#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <memory>

namespace mu2e {

  class STMDepositEnergy : public art::EDAnalyzer {
  public:
    struct Config {
      fhicl::Atom<std::string> stepPointMCTag { fhicl::Name("stepPointMCTag"), 
                                               fhicl::Comment("Tag for StepPointMC collection") };
      fhicl::Atom<int> verboseLevel { fhicl::Name("verboseLevel"), 
                                     fhicl::Comment("Verbose level"), 0 };
      fhicl::Atom<bool> saveToTree { fhicl::Name("saveToTree"), 
                                    fhicl::Comment("Save results to TTree"), true };
      fhicl::Atom<std::string> treeName { fhicl::Name("treeName"), 
                                         fhicl::Comment("Name of the output TTree"), "EnergyDeposits" };
      fhicl::Atom<bool> groupByVolume { fhicl::Name("groupByVolume"), 
                                       fhicl::Comment("Group energy deposits by volume ID"), false };
      fhicl::Atom<std::string> outputFileName { fhicl::Name("outputFileName"), 
                                              fhicl::Comment("Name of the output ROOT file"), "" };
    };
    
    using Parameters = art::EDAnalyzer::Table<Config>;
    explicit STMDepositEnergy(const Parameters& conf);
    
    void analyze(const art::Event& event) override;
    void beginJob() override;
    void endJob() override;
    
  private:
    art::InputTag _stepPointMCTag;
    int _verboseLevel;
    bool _saveToTree;
    std::string _treeName;
    bool _groupByVolume;
    std::string _outputFileName;
    
    struct EnergyDeposits {
      double total = 0.0;
      double visible = 0.0;
      double nonIonizing = 0.0;
    };
    
    EnergyDeposits _totalEnergySum;
    int _totalEvents = 0;
    
    std::unique_ptr<TFile> _outputFile;
    TTree* _tree = nullptr;
    
    int _eventId;
    int _runId;
    int _subRunId;
    double _totalEnergy;
    double _visibleEnergy;
    double _nonIonizingEnergy;
    std::vector<int> _volumeIds;
    std::vector<double> _volumeTotalEnergies;
    std::vector<double> _volumeVisibleEnergies;
    std::vector<double> _volumeNonIonizingEnergies;
    
    TH1F* _hTotalEnergy = nullptr;
    TH1F* _hVisibleEnergy = nullptr;
    TH1F* _hNonIonizingEnergy = nullptr;
  };

  STMDepositEnergy::STMDepositEnergy(const Parameters& conf) :
    art::EDAnalyzer(conf),
    _stepPointMCTag(conf().stepPointMCTag()),
    _verboseLevel(conf().verboseLevel()),
    _saveToTree(conf().saveToTree()),
    _treeName(conf().treeName()),
    _groupByVolume(conf().groupByVolume()),
    _outputFileName(conf().outputFileName())
  {
    std::cout << "STMDepositEnergy initialized with tag: " 
              << _stepPointMCTag << std::endl;
    if (!_outputFileName.empty()) {
      std::cout << "Output will be saved to: " << _outputFileName << std::endl;
    }
  }
  
  void STMDepositEnergy::beginJob() {
    if (_saveToTree) {
      if (!_outputFileName.empty()) {
        _outputFile = std::make_unique<TFile>(_outputFileName.c_str(), "RECREATE");
      } else {
        art::ServiceHandle<art::TFileService> tfs;
        _tree = tfs->make<TTree>(_treeName.c_str(), "Detector Energy Deposits");
        _hTotalEnergy = tfs->make<TH1F>("hTotalEnergy", "Total Energy Deposit;Energy (MeV);Events", 100, 0, 100);
        _hVisibleEnergy = tfs->make<TH1F>("hVisibleEnergy", "Visible Energy Deposit;Energy (MeV);Events", 100, 0, 100);
        _hNonIonizingEnergy = tfs->make<TH1F>("hNonIonizingEnergy", "Non-Ionizing Energy Deposit;Energy (MeV);Events", 100, 0, 20);
        return;
      }
      
      _tree = new TTree(_treeName.c_str(), "Detector Energy Deposits");
      _hTotalEnergy = new TH1F("hTotalEnergy", "Total Energy Deposit;Energy (MeV);Events", 100, 0, 100);
      _hVisibleEnergy = new TH1F("hVisibleEnergy", "Visible Energy Deposit;Energy (MeV);Events", 100, 0, 100);
      _hNonIonizingEnergy = new TH1F("hNonIonizingEnergy", "Non-Ionizing Energy Deposit;Energy (MeV);Events", 100, 0, 20);
      
      _tree->Branch("eventId", &_eventId, "eventId/I");
      _tree->Branch("runId", &_runId, "runId/I");
      _tree->Branch("subRunId", &_subRunId, "subRunId/I");
      _tree->Branch("totalEnergy", &_totalEnergy, "totalEnergy/D");
      _tree->Branch("visibleEnergy", &_visibleEnergy, "visibleEnergy/D");
      _tree->Branch("nonIonizingEnergy", &_nonIonizingEnergy, "nonIonizingEnergy/D");
      
      if (_groupByVolume) {
        _tree->Branch("volumeIds", &_volumeIds);
        _tree->Branch("volumeTotalEnergies", &_volumeTotalEnergies);
        _tree->Branch("volumeVisibleEnergies", &_volumeVisibleEnergies);
        _tree->Branch("volumeNonIonizingEnergies", &_volumeNonIonizingEnergies);
      }
    }
  }

  void STMDepositEnergy::analyze(const art::Event& event) {
    _eventId = event.id().event();
    _runId = event.id().run();
    _subRunId = event.id().subRun();
    
    auto stepsHandle = event.getValidHandle<StepPointMCCollection>(_stepPointMCTag);
    const auto& steps = *stepsHandle;
    
    EnergyDeposits energyDeposits;
    
    std::map<int, EnergyDeposits> volumeEnergyMap;
    
    for (const auto& step : steps) {
      energyDeposits.total += step.totalEDep();
      energyDeposits.visible += step.visibleEDep();
      energyDeposits.nonIonizing += step.nonIonizingEDep();
      
      if (_groupByVolume) {
        int volumeId = step.volumeId();
        volumeEnergyMap[volumeId].total += step.totalEDep();
        volumeEnergyMap[volumeId].visible += step.visibleEDep();
        volumeEnergyMap[volumeId].nonIonizing += step.nonIonizingEDep();
      }
    }
    
    _totalEnergySum.total += energyDeposits.total;
    _totalEnergySum.visible += energyDeposits.visible;
    _totalEnergySum.nonIonizing += energyDeposits.nonIonizing;
    _totalEvents++;
    
    if (_verboseLevel > 0) {
      std::cout << "==== Event " << _eventId << " Energy Deposits in " << _stepPointMCTag.instance() << " ====" << std::endl;
      std::cout << "Total energy deposit: " << energyDeposits.total << " MeV" << std::endl;
      std::cout << "Visible energy deposit: " << energyDeposits.visible << " MeV" << std::endl;
      std::cout << "Non-ionizing energy deposit: " << energyDeposits.nonIonizing << " MeV" << std::endl;
      
      if (_verboseLevel > 1 && _groupByVolume) {
        std::cout << "\nEnergy deposits by volume ID:" << std::endl;
        for (const auto& [volumeId, energy] : volumeEnergyMap) {
          std::cout << "Volume " << volumeId 
                    << ": Total = " << energy.total << " MeV"
                    << ", Visible = " << energy.visible << " MeV"
                    << ", NonIonizing = " << energy.nonIonizing << " MeV" 
                    << std::endl;
        }
      }
      
      std::cout << "========================================" << std::endl;
    }
    
    if (_saveToTree && _tree) {
      _totalEnergy = energyDeposits.total;
      _visibleEnergy = energyDeposits.visible;
      _nonIonizingEnergy = energyDeposits.nonIonizing;
      
      _volumeIds.clear();
      _volumeTotalEnergies.clear();
      _volumeVisibleEnergies.clear();
      _volumeNonIonizingEnergies.clear();
      
      if (_groupByVolume) {
        for (const auto& [volumeId, energy] : volumeEnergyMap) {
          _volumeIds.push_back(volumeId);
          _volumeTotalEnergies.push_back(energy.total);
          _volumeVisibleEnergies.push_back(energy.visible);
          _volumeNonIonizingEnergies.push_back(energy.nonIonizing);
        }
      }
      
      if (_hTotalEnergy) _hTotalEnergy->Fill(energyDeposits.total);
      if (_hVisibleEnergy) _hVisibleEnergy->Fill(energyDeposits.visible);
      if (_hNonIonizingEnergy) _hNonIonizingEnergy->Fill(energyDeposits.nonIonizing);
      
      _tree->Fill();
    }
  }
  
  void STMDepositEnergy::endJob() {
    if (_totalEvents > 0) {
      std::cout << "\n===== Summary of Energy Deposits in " << _stepPointMCTag.instance() << " =====" << std::endl;
      std::cout << "Total events processed: " << _totalEvents << std::endl;
      std::cout << "Average total energy deposit: " << _totalEnergySum.total / _totalEvents << " MeV" << std::endl;
      std::cout << "Average visible energy deposit: " << _totalEnergySum.visible / _totalEvents << " MeV" << std::endl;
      std::cout << "Average non-ionizing energy deposit: " << _totalEnergySum.nonIonizing / _totalEvents << " MeV" << std::endl;
      std::cout << "=======================================" << std::endl;
    }
    
    if (_outputFile) {
      _outputFile->cd();
      if (_tree) _tree->Write();
      if (_hTotalEnergy) _hTotalEnergy->Write();
      if (_hVisibleEnergy) _hVisibleEnergy->Write();
      if (_hNonIonizingEnergy) _hNonIonizingEnergy->Write();
      _outputFile->Close();
    }
  }
}

DEFINE_ART_MODULE(mu2e::STMDepositEnergy)


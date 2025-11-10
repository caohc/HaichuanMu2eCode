#include <iostream>
#include <string>

// art includes
#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"

// fhicl includes
#include "fhiclcpp/types/Atom.h"
#include "fhiclcpp/types/Sequence.h"
#include "fhiclcpp/types/OptionalAtom.h"
#include "canvas/Utilities/InputTag.h"

// Offline includes
#include "Offline/MCDataProducts/inc/SimParticle.hh"
#include "Offline/MCDataProducts/inc/StepPointMC.hh"
#include "Offline/MCDataProducts/inc/MCRelationship.hh"
#include "Offline/MCDataProducts/inc/PrimaryParticle.hh"

//using namespace std;

namespace mu2e{
  class STMResamplingProducer : public art::EDProducer
  {
  public:
    using Name=fhicl::Name;
    using Comment=fhicl::Comment;

    struct Config
    {
      fhicl::Atom<std::string> stepPointMCsTag{Name("RelationStepPointMCsTag"), Comment("Input tag of StepPointMCs associated with VD10 and VD116")};
      fhicl::OptionalAtom<bool> verbose{Name("verbose"), Comment("Verbosity of output")};
    };

    using Parameters=art::EDProducer::Table<Config>;

    explicit STMResamplingProducer(const Parameters& pset);
    virtual void produce(art::Event& event) override;
  private:
    art::ProductToken<StepPointMCCollection> _stepPointMCsToken;
    bool _verbose = false;
    uint _keptStepPointMCCounter = 0;
  };
  // ===================================================
  STMResamplingProducer::STMResamplingProducer(const Parameters& conf) :
    art::EDProducer{conf},
    _stepPointMCsToken(consumes<StepPointMCCollection>(conf().stepPointMCsTag()))
    {
      produces<StepPointMCCollection>();
      if (conf().verbose.hasValue()) {_verbose = *std::move(conf().verbose());}
      else {_verbose = false;}
    };
  // ===================================================


  //int n1=0;
  //int n2=0;

  void STMResamplingProducer::produce(art::Event& event)
  {
    auto const& StepPointMCs = event.getProduct(_stepPointMCsToken);
    auto const& StepPointMCs2 = event.getProduct(_stepPointMCsToken);

    // Define the StepPointMCCollection to be added to the event
    std::unique_ptr<StepPointMCCollection> _output_StepPointMCs(new StepPointMCCollection);

    // Check if the event has a hit in VirtualDetectorFilterID. If so add it to the collection
    bool flag=0;
    bool repeat=0;
    int count=0;

    StepPointMC vd10_particle[3];

    for (const StepPointMC& step : StepPointMCs)
   {
       	 flag=0;
	 count=0;
         if ( step.volumeId() == 116)
	{
	  flag=1;
	}
 
 	 if (flag==1)
        { 
	
     	   for (const StepPointMC& step2 : StepPointMCs2)
          {
             if ( step2.volumeId() == 10)
	    {

                 MCRelationship relation(step2.simParticle(), step.simParticle());
                 if (relation.relationship() == MCRelationship::same)
		 {
                   vd10_particle[0] = step2;
		   count=1;
		   break;
		 }	 
			
		 else if (relation.relationship() == MCRelationship::mother || relation.relationship() == MCRelationship::umother)
                {
                
		    if(count>0)
	           {
                      for(int ll=0; ll<count; ll++)
		     {
                         if(step2.simParticle().key()==vd10_particle[ll].simParticle().key())
			{
                          repeat=1; break;
                        }                   
	             }

                      if(repeat==0) {vd10_particle[count]  = step2; count++;}
		      else repeat=0;		      
                   }
                   
                    else if(count==0) {vd10_particle[0]  = step2; count++;}
			  
                }
	    }
	  }

	    if(count==0)
	    {
               _output_StepPointMCs->emplace_back(step);
	    }

            if(count==1) 
	    {
		    
                 _output_StepPointMCs->emplace_back(step);
                 _output_StepPointMCs->emplace_back(vd10_particle[0]);

	      //std::cout<<count<<"\t"<<++n1<<"\t"<<step.simParticle()->pdgId()<<"\t"<<step.simParticle().key()<<"\t"<<vd10_particle[0].simParticle()->pdgId()<<"\t"<<vd10_particle[0].simParticle().key()<<std::endl;
	    }

            else if(count==2)
           {
              MCRelationship relation2(vd10_particle[0].simParticle(), vd10_particle[1].simParticle());
              if (relation2.relationship() == MCRelationship::daughter || relation2.relationship() == MCRelationship::udaughter)
	     {  
                 _output_StepPointMCs->emplace_back(step);
                 _output_StepPointMCs->emplace_back(vd10_particle[0]);
	        //std::cout<<count<<"\t"<<++n2<<"\t"<<step.simParticle()->pdgId()<<"\t"<<step.simParticle().key()<<"\t"<<vd10_particle[0].simParticle()->pdgId()<<"\t"<<vd10_particle[0].simParticle().key()<<std::endl;
	     }


              else if (relation2.relationship() == MCRelationship::mother || relation2.relationship() == MCRelationship::umother)
	      {

                 _output_StepPointMCs->emplace_back(step);
                 _output_StepPointMCs->emplace_back(vd10_particle[1]);
		//std::cout<<count<<"\t"<<++n2<<"\t"<<step.simParticle()->pdgId()<<"\t"<<step.simParticle().key()<<"\t"<<vd10_particle[1].simParticle()->pdgId()<<"\t"<<vd10_particle[1].simParticle().key()<<std::endl;
	      }

              else
             {
                 std::cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<std::endl;
                 std::cout<<"Error22222!!!"<<std::endl;
                 std::cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<std::endl;
             }

           }

            else if(count==3)
           {
              std::cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<std::endl;
              std::cout<<"Error33333!!!"<<std::endl;
              std::cout<<count<<"\t"<<step.simParticle()->pdgId()<<"\t"<<step.simParticle().key()<<std::endl;
              std::cout<<vd10_particle[0].simParticle()->pdgId()<<"\t"<<vd10_particle[0].simParticle().key()<<"\t"<<vd10_particle[1].simParticle()->pdgId()<<"\t"<<vd10_particle[1].simParticle().key()<<"\t"<<vd10_particle[2].simParticle()->pdgId()<<"\t"<<vd10_particle[2].simParticle().key()<<"\t"<<std::endl;
	      std::cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<std::endl;
           }


  	}
    }

    _keptStepPointMCCounter += _output_StepPointMCs->size();
    event.put(std::move(_output_StepPointMCs));

    // return;
  };
}

DEFINE_ART_MODULE(mu2e::STMResamplingProducer)

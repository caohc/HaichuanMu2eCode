#include <iostream>
#include <string>

// art includes
#include "art/Framework/Core/EDFilter.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"

// fhicl includes
#include "fhiclcpp/types/Atom.h"
#include "fhiclcpp/types/Sequence.h"
#include "fhiclcpp/types/OptionalAtom.h"
#include "canvas/Utilities/InputTag.h"

// Offline includes
#include "Offline/MCDataProducts/inc/StepPointMC.hh"
#include "Offline/MCDataProducts/inc/MCRelationship.hh"

using namespace std;

namespace mu2e{
  class STMResamplingFilter : public art::EDFilter
  {
  public:
    using Name=fhicl::Name;
    using Comment=fhicl::Comment;

    struct Config
    {
      fhicl::Atom<art::InputTag> stepPointMCsTag{Name("RelationStepPointMCsTag"), Comment("Input tag of StepPointMCs associated with VD10 and VD116")};
      fhicl::Atom<std::string> particleSource{Name("particleSource"), Comment("")};
    };

    using Parameters=art::EDFilter::Table<Config>;

    explicit STMResamplingFilter(const Parameters& pset);
    virtual bool filter(art::Event& event) override;

  private:
    art::ProductToken<StepPointMCCollection> _stepPointMCsToken;
    std::string  _particleSource;

  };
  // ===================================================
  STMResamplingFilter::STMResamplingFilter(const Parameters& conf) :
    art::EDFilter{conf},
    _stepPointMCsToken(consumes<StepPointMCCollection>(conf().stepPointMCsTag())),
    _particleSource(conf().particleSource())
    {};
  // ===================================================
  bool STMResamplingFilter::filter(art::Event& event)
  {

    auto const& StepPointMCs = event.getProduct(_stepPointMCsToken);
    auto const& StepPointMCs2 = event.getProduct(_stepPointMCsToken);

    bool flag=0;
    bool repeat=0;
    int count=0;

    StepPointMC vd10_particle[3];

    for (const StepPointMC& step : StepPointMCs)
   {
         flag=0;
         count=0;
         if ( step.volumeId() == 116) flag=1;

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
                 if(_particleSource=="NotVD10") return true;
             }	   

 	      else if(count==1)
            {

              //std::cout<<step.simParticle()->pdgId()<<"\t"<<step.simParticle().key()<<"\t"<<vd10_particle[0].simParticle()->pdgId()<<"\t"<<vd10_particle[0].simParticle().key()<<std::endl;

	      if(_particleSource =="fromGamma" && vd10_particle[0].simParticle()->pdgId()==22) return true;		    
	      else if(_particleSource =="fromEpm" && abs(vd10_particle[0].simParticle()->pdgId())==11) return true;
              else if(_particleSource =="fromOthers" && abs(vd10_particle[0].simParticle()->pdgId()) != 11 && vd10_particle[0].simParticle()->pdgId() !=22 ) return true; 
            }

            else if(count==2)
           {
              MCRelationship relation2(vd10_particle[0].simParticle(), vd10_particle[1].simParticle());
              if (relation2.relationship() == MCRelationship::same || relation2.relationship() == MCRelationship::daughter || relation2.relationship() == MCRelationship::udaughter)
             {
              if(_particleSource =="fromGamma" && vd10_particle[0].simParticle()->pdgId()==22) return true;        
              else if(_particleSource =="fromEpm" && abs(vd10_particle[0].simParticle()->pdgId())==11) return true;
              else if(_particleSource =="fromOthers" && abs(vd10_particle[0].simParticle()->pdgId()) != 11 && vd10_particle[0].simParticle()->pdgId() !=22) return true;
	     }

              else if (relation2.relationship() == MCRelationship::mother || relation2.relationship() == MCRelationship::umother)
             {
              if(_particleSource =="fromGamma" && vd10_particle[1].simParticle()->pdgId()==22) return true;        
              else if(_particleSource =="fromEpm" && abs(vd10_particle[1].simParticle()->pdgId())==11) return true;
              else if(_particleSource =="fromOthers" && abs(vd10_particle[1].simParticle()->pdgId()) != 11 && vd10_particle[1].simParticle()->pdgId() !=22) return true;
	     }

              else
             {
                 std::cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<std::endl;
                 std::cout<<"Error22222!!!"<<std::endl;
                 std::cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<std::endl;
                 return false;
	     }

           }

            else if(count>=3)
           {
              std::cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<std::endl;
              std::cout<<"Error33333!!!"<<std::endl;
              std::cout<<count<<"\t"<<step.simParticle()->pdgId()<<"\t"<<step.simParticle().key()<<std::endl;
              std::cout<<vd10_particle[0].simParticle()->pdgId()<<"\t"<<vd10_particle[0].simParticle().key()<<"\t"<<vd10_particle[1].simParticle()->pdgId()<<"\t"<<vd10_particle[1].simParticle().key()<<"\t"<<vd10_particle[2].simParticle()->pdgId()<<"\t"<<vd10_particle[2].simParticle().key()<<"\t"<<std::endl;
              std::cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<std::endl;
              return false;
           }
        }
	 else return false;
    }

    return false;
  }



  // ===================================================
}

DEFINE_ART_MODULE(mu2e::STMResamplingFilter)

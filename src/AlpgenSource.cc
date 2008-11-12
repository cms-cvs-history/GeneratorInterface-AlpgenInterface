/*
 *  $Date: 2008/06/04 21:57:05 $
 *  $Revision: 1.15 $
 *  
 *  Filip Moorgat & Hector Naves 
 *  26/10/05
 * 
 *  Patrick Janot : added the PYTHIA card reading
 *
 *  Serge SLabospitsky : added Alpgen reading tools 
 */

#include "GeneratorInterface/AlpgenInterface/interface/AlpgenSource.h"
#include "GeneratorInterface/AlpgenInterface/interface/PYR.h"
#include "SimDataFormats/HepMCProduct/interface/HepMCProduct.h"
#include "SimDataFormats/GeneratorProducts/interface/LesHouches.h"
#include "SimDataFormats/GeneratorProducts/interface/LHECommonBlocks.h"
#include "SimDataFormats/GeneratorProducts/interface/LHERunInfoProduct.h"
#include "SimDataFormats/GeneratorProducts/interface/LHEEventProduct.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/Run.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Utilities/interface/RandomNumberGenerator.h"

#include "time.h"

using namespace edm; 
using namespace std;

// Generator modifications
// ***********************
#include "HepMC/PythiaWrapper6_2.h"
#include "HepMC/IO_HEPEVT.h"

//#include "GeneratorInterface/CommonInterface/interface/PretauolaWrapper.h"
//#include "CLHEP/HepMC/ConvertHEPEVT.h"
//#include "CLHEP/HepMC/CBhepevt.h"

#include "GeneratorInterface/CommonInterface/interface/PythiaCMS.h"
#include "GeneratorInterface/CommonInterface/interface/Txgive.h"

HepMC::IO_HEPEVT conv;
// ***********************


//used for defaults
  static const unsigned long kNanoSecPerSec = 1000000000;
  static const unsigned long kAveEventPerSec = 200;

AlpgenSource::AlpgenSource( const ParameterSet & pset, 
			    InputSourceDescription const& desc ) :
  ExternalInputSource(pset, desc, false), evt(0), 
  pythiaPylistVerbosity_ (pset.getUntrackedParameter<int>("pythiaPylistVerbosity",0)),
  pythiaHepMCVerbosity_ (pset.getUntrackedParameter<bool>("pythiaHepMCVerbosity",false)),
  maxEventsToPrint_ (pset.getUntrackedParameter<int>("maxEventsToPrint",1)),
  lheAlpgenUnwParHeader("AlpgenUnwParFile")
{
  
  fileName_ = fileNames()[0];
  // strip the file: 
  if ( fileName_.find("file:") || fileName_.find("rfio:")){
    fileName_.erase(0,5);
  }   

  // open the .unw file to store additional 
  // informations in the AlpgenInfoProduct
//  unwfile = new ifstream((fileName_+".unw").c_str());
  // get the number of input events from  _unw.par files
  char buffer[256];
  ifstream reader((fileName_+"_unw.par").c_str());
  char sNev[80];
  lheAlpgenUnwParHeader.addLine("\n");
  while ( reader.getline (buffer,256) ) {
    istringstream is(buffer);
    lheAlpgenUnwParHeader.addLine(std::string(buffer) + "\n");
    is >> sNev;
    Nev_ = atoi(sNev);
  }

  //check that N(asked events) <= N(input events)
  if(maxEvents()>Nev_) {
    cout << "ALPGEN warning: Number of events requested > Number of unweighted events" << endl;
    cout << "                Execution will be stoped after processing the last unweighted event" << endl;  
  }

  if(maxEvents() != -1 && maxEvents() < Nev_) // stop at N(asked events) if N(asked events)<N(input events)
    Nev_ = maxEvents();
  
  // PYLIST Verbosity Level
  // Valid PYLIST arguments are: 1, 2, 3, 5, 7, 11, 12, 13
  pythiaPylistVerbosity_ = pset.getUntrackedParameter<int>("pythiaPylistVerbosity",0);
  
  // HepMC event verbosity Level
  pythiaHepMCVerbosity_ = pset.getUntrackedParameter<bool>("pythiaHepMCVerbosity",false);
  
  //Max number of events printed on verbosity level 
  maxEventsToPrint_ = pset.getUntrackedParameter<int>("maxEventsToPrint",0);
  
  // Set PYTHIA parameters in a single ParameterSet
  {
    ParameterSet pythia_params = 
      pset.getParameter<ParameterSet>("PythiaParameters") ;
    
    // Read the PYTHIA parameters for each set of parameters
    vector<string> pars = 
      pythia_params.getParameter<vector<string> >("pythia");
    
    // Loop over all parameters and stop in case of mistake
    for( vector<string>::const_iterator  
	   itPar = pars.begin(); itPar != pars.end(); ++itPar ) {
      static string sRandomValueSetting("MRPY(1)");
      if( 0 == itPar->compare(0,sRandomValueSetting.size(),sRandomValueSetting) ) {
	throw edm::Exception(edm::errors::Configuration,"PythiaError")
	  <<" attempted to set random number seed.";
      }
      if( ! call_pygive(*itPar) ) {
	throw edm::Exception(edm::errors::Configuration,"PythiaError") 
	  <<" pythia did not accept the following \""<<*itPar<<"\"";
      }
    }
  }

  // Read the Alpgen parameters

  // read External Generator parameters
  {   ParameterSet generator_params = 
	pset.getParameter<ParameterSet>("GeneratorParameters") ;
  vector<string> pars = 
    generator_params.getParameter<vector<string> >("generator");
  for( vector<string>::const_iterator  
	 itPar = pars.begin(); itPar != pars.end(); ++itPar ) 
    {
      call_txgive(*itPar);          
    }
  // giving to txgive a string with the alpgen filename
  string tmpstring = "UNWFILE = " + fileName_;
  call_txgive(tmpstring);
  }
  
  
  //In the future, we will get the random number seed on each event and tell 
  // pythia to use that new seed
  edm::Service<RandomNumberGenerator> rng;
  randomEngine = fRandomEngine = &(rng->getEngine());
  uint32_t seed = rng->mySeed();
  ostringstream sRandomSet;
  sRandomSet <<"MRPY(1)="<<seed;
  call_pygive(sRandomSet.str());
  
  //  call_pretauola(-1);     // TAUOLA initialization
  call_pyinit( "USER", "p", "p", 14000. );
  
  cout << endl; // Stetically add for the output
  //********                                      
  
  produces<HepMCProduct>();
  produces<LHEEventProduct>();

//  produces<AlpWgtFileInfoProduct, edm::InRun>();
  produces<LHERunInfoProduct, edm::InRun>();
}


AlpgenSource::~AlpgenSource(){
  call_pystat(1);
  //  call_pretauola(1);  // output from TAUOLA 
  alpgen_end();
  clear(); 
}

void AlpgenSource::clear() {
  
}

void AlpgenSource::beginRun(Run & r) {
  // get the LHE init information from Fortran code
  lhef::HEPRUP heprup;
  lhef::CommonBlocks::readHEPRUP(&heprup);
  auto_ptr<LHERunInfoProduct> runInfo(new LHERunInfoProduct(heprup));

  // information on weighted events
  LHERunInfoProduct::Header lheAlpgenWgtHeader("AlpgenWgtFile");
  lheAlpgenWgtHeader.addLine("\n");
  ifstream wgtascii((fileName_+".wgt").c_str());
  char buffer[512];
  while(wgtascii.getline(buffer,512)) {
    lheAlpgenWgtHeader.addLine(std::string(buffer) + "\n");
  }

  // comments on top
  LHERunInfoProduct::Header comments;
  comments.addLine("\n");
  comments.addLine("\tExtracted by AlpgenInterface\n");

  // build the final Run info object
  runInfo->addHeader(comments);
  runInfo->addHeader(lheAlpgenUnwParHeader);
  runInfo->addHeader(lheAlpgenWgtHeader);
  r.put(runInfo);
}

bool AlpgenSource::produce(Event & e) {
  
  // exit if N(events asked) has been exceeded
  if(event()> Nev_) {
    return false;
  } else {
    
    auto_ptr<HepMCProduct> bare_product(new HepMCProduct());  
    
//    // Additional information from unweighted file
//    auto_ptr<AlpgenInfoProduct> alp_product(new AlpgenInfoProduct());

    // Extract from .unw file the info for AlpgenInfoProduct
    
//    char buffer[512];
//    if(unwfile->getline(buffer,512)) {
//      alp_product->EventInfo(buffer);
//    }
//    if(unwfile->getline(buffer,512)) 
//      alp_product->InPartonInfo(buffer);
//    if(unwfile->getline(buffer,512)) 
//      alp_product->InPartonInfo(buffer);
//    for(int i_out = 0; i_out <  alp_product->nTot()-2; i_out++) {
//      if(unwfile->getline(buffer,512)) 
//	alp_product->OutPartonInfo(buffer);
//    }

    call_pyevnt();      // generate one event with Pythia
    //        call_pretauola(0);  // tau-lepton decays with TAUOLA 

    // fill the parton level LHE event information
    lhef::HEPEUP hepeup;
    lhef::CommonBlocks::readHEPEUP(&hepeup);
    hepeup.AQEDUP = hepeup.AQCDUP = -1.0; // alphas are not saved by Alpgen
    for(int i = 0; i < hepeup.NUP; i++)
      hepeup.SPINUP[i] = -9;	// Alpgen does not store spin information
    auto_ptr<LHEEventProduct> lheEvent(new LHEEventProduct(hepeup));
    e.put(lheEvent);

    call_pyhepc( 1 );
    
    //    HepMC::GenEvent* evt = conv.getGenEventfromHEPEVT();
    HepMC::GenEvent* evt = conv.read_next_event();
    
    evt->set_signal_process_id(pypars.msti[0]);
    evt->set_event_number(numberEventsInRun() - remainingEvents() - 1);
    
    int id1 = pyint1.mint[14];
    int id2 = pyint1.mint[15];
    if ( id1 == 21 ) id1 = 0;
    if ( id2 == 21 ) id2 = 0; 
    double x1 = pyint1.vint[40];
    double x2 = pyint1.vint[41];  
    double Q  = pyint1.vint[50];
    double pdf1 = pyint1.vint[38];
    pdf1 /= x1 ;
    double pdf2 = pyint1.vint[39];
    pdf2 /= x2 ;
    evt->set_pdf_info( HepMC::PdfInfo(id1,id2,x1,x2,Q,pdf1,pdf2) ) ;
    
    evt->weights().push_back( pyint1.vint[96] );

    //******** Verbosity ********
    
    if(event() <= maxEventsToPrint_ &&
       (pythiaPylistVerbosity_ || pythiaHepMCVerbosity_)) {
      
      // Prints PYLIST info
      if(pythiaPylistVerbosity_) {
	call_pylist(pythiaPylistVerbosity_);
      }
      
      // Prints HepMC event
      if(pythiaHepMCVerbosity_) {
	cout << "Event process = " << pypars.msti[0] << endl 
	     << "----------------------" << endl;
	evt->print();
      }
    }
    
    //evt = reader_->fillCurrentEventData(); 
    //********                                      
    
    if(evt)  bare_product->addHepMCData(evt );
    
    e.put(bare_product);
//    e.put(alp_product);

    return true;
  }
}

bool 
AlpgenSource::call_pygive(const std::string& iParm ) {

  int numWarn = pydat1.mstu[26]; //# warnings
  int numErr = pydat1.mstu[22];// # errors
  
//call the fortran routine pygive with a fortran string
  PYGIVE( iParm.c_str(), iParm.length() );  
  //  PYGIVE( iParm );  
//if an error or warning happens it is problem
  return pydat1.mstu[26] == numWarn && pydat1.mstu[22] == numErr;   
}
//------------
bool 
AlpgenSource::call_txgive(const std::string& iParm ) 
   {
    //call the fortran routine txgive with a fortran string
    TXGIVE( iParm.c_str(), iParm.length() );  
    return 1;  
   }

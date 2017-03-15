// system include files
#include <memory>
#include <cmath>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "DataFormats/Math/interface/deltaR.h"
#include "FWCore/Common/interface/TriggerNames.h"
#include "DataFormats/Common/interface/TriggerResults.h"
// #include "DataFormats/PatCandidates/interface/TriggerObjectStandAlone.h"
// #include "DataFormats/PatCandidates/interface/PackedTriggerPrescales.h"

#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "DataFormats/L1Trigger/interface/EGamma.h"
#include "DataFormats/L1Trigger/interface/Muon.h"

#include "DataFormats/MuonReco/interface/Muon.h"
#include "DataFormats/MuonReco/interface/MuonFwd.h"
#include "DataFormats/EgammaCandidates/interface/Photon.h"
#include "DataFormats/EgammaCandidates/interface/PhotonFwd.h"

#include "DataFormats/HLTReco/interface/TriggerObject.h"
#include "DataFormats/HLTReco/interface/TriggerEvent.h"


#include <map>
#include <string>
#include <stdlib.h>
#include <iostream>
#include <fstream>



class AODTriggerAnalyzer : public edm::EDAnalyzer {
	public:
		explicit AODTriggerAnalyzer(const edm::ParameterSet&);
		trigger::TriggerObjectCollection filterFinder(edm::EDGetTokenT<trigger::TriggerEvent> triggerSummaryLabel, edm::InputTag filterTag, const edm::Event &iEvent);
		bool l1Filter(edm::Handle< BXVector<l1t::Muon> > l1Muons, edm::Handle< BXVector<l1t::EGamma> > l1EGammas, const edm::Event &iEvent);
		bool recoFilter(edm::Handle< reco::MuonCollection > recoMuons, edm::Handle< reco::PhotonCollection > recoPhotons, const edm::Event &iEvent);
		bool hltFilter(trigger::TriggerObjectCollection muonL3Objects, trigger::TriggerObjectCollection photonL3Objects, const edm::Event &iEvent);

		~AODTriggerAnalyzer() {}



	private:
		virtual void analyze(const edm::Event&, const edm::EventSetup&) override;
		bool verbose; 
		edm::EDGetTokenT< edm::TriggerResults > triggerBits_;
		edm::EDGetTokenT< BXVector<l1t::Muon> > l1Muons_;
		edm::EDGetTokenT< BXVector<l1t::EGamma> > l1EGammas_;
		edm::EDGetTokenT< reco::MuonCollection > recoMuons_;
		edm::EDGetTokenT< reco::PhotonCollection > recoPhotons_;
		edm::EDGetTokenT< trigger::TriggerEvent > triggerSummaryLabel_;
		edm::InputTag muonFilterTag_;
		edm::InputTag photonFilterTag_;

		// Reco configs
		double minMuPt;
		double maxMuEta;
		double muonLeadPt, muonTrailPt;
		double minJPsiMass ;
		double maxJPsiMass,GammaMinPtCut,drLeadMuPhotonSel,drTrailPhotonSel;
		// L1 Configs
		bool l1MuonOS_;
		bool l1MuonIso_;
		int l1MuonQltMin_;
		int l1MuonQltMax_;
		std::vector<double> l1MuonPt_;




		// edm::EDGetTokenT<pat::TriggerObjectStandAloneCollection> triggerObjects_;
		// edm::EDGetTokenT<pat::PackedTriggerPrescales> triggerPrescales_;
};

AODTriggerAnalyzer::AODTriggerAnalyzer(const edm::ParameterSet& iConfig):
	verbose (iConfig.getParameter< bool > ("Verbose")),
	triggerBits_(consumes< edm::TriggerResults >(iConfig.getParameter<edm::InputTag>("bits"))),
	l1Muons_(consumes< BXVector<l1t::Muon> >(iConfig.getParameter<edm::InputTag>("l1MuonsLabel"))),
	l1EGammas_(consumes< BXVector<l1t::EGamma> >(iConfig.getParameter<edm::InputTag>("l1EGammasLabel"))),
	recoMuons_(consumes< reco::MuonCollection >(iConfig.getParameter<edm::InputTag>("recoMuonsLabel"))),
	recoPhotons_(consumes< reco::PhotonCollection >(iConfig.getParameter<edm::InputTag>("recoPhotonsLabel"))),
	triggerSummaryLabel_ (consumes<trigger::TriggerEvent>(iConfig.getParameter<edm::InputTag> ("triggerSummaryLabel"))),
	muonFilterTag_ (iConfig.getParameter<edm::InputTag> ("muonFilterTag")),
	photonFilterTag_ (iConfig.getParameter<edm::InputTag> ("photonFilterTag")),

	// Reco config
	minMuPt (iConfig.getUntrackedParameter<double>("minMuPt",2.0)),
	maxMuEta (iConfig.getUntrackedParameter<double>("maxMuEta",2.4)), 
	muonLeadPt (iConfig.getUntrackedParameter<double>("minMuonLeadPt",20.0)),
	muonTrailPt (iConfig.getUntrackedParameter<double>("minMuonTrailPt",4.0)),
	minJPsiMass (iConfig.getUntrackedParameter<double>("minJPsiMass",2.95)),
	maxJPsiMass (iConfig.getUntrackedParameter<double>("maxJPsiMass",3.25)),
	GammaMinPtCut (iConfig.getUntrackedParameter<double>("GammaMinPtCut",0.1)),
	drLeadMuPhotonSel (iConfig.getUntrackedParameter<double>("DeltaRLeadMuPhotonSel",1.0)),
	drTrailPhotonSel (iConfig.getUntrackedParameter<double>("DeltaRTrailPhotonSel",1.0)),  

	// L1 Configs
	l1MuonOS_ (iConfig.getParameter< bool > ("l1MuonOS")),
	l1MuonIso_ (iConfig.getParameter< bool > ("l1MuonIso")),
	l1MuonQltMin_ (iConfig.getParameter< int > ("l1MuonQltMin")),
	l1MuonQltMax_ (iConfig.getParameter< int > ("l1MuonQltMax")),
	l1MuonPt_ (iConfig.getParameter< std::vector<double> > ("l1MuonPt"))

{
	edm::Service<TFileService> fs;
	// TH1D* effHist =  fs->make<TH1D>(TString(histograms_[i].getParameter<string>("variable")),TString(histograms_[i].getParameter<string>("variable")),int(histograms_[i].getParameter<int>("nBins")),histograms_[i].getParameter<double>("lBin"),histograms_[i].getParameter<double>("hBin"));     
	// TH1D* tmpMatchedHist = fs->make<TH1D>(TString(histograms_[i].getParameter<string>("variable") + "_filterMatched"),TString(histograms_[i].getParameter<string>("variable") + "_filterMatched"),int(histograms_[i].getParameter<int>("nBins")),histograms_[i].getParameter<double>("lBin"),histograms_[i].getParameter<double>("hBin"));
	// TH1D* tmpEffHist = fs->make<TH1D>(TString(histograms_[i].getParameter<string>("variable") + "_filterEff"),TString(histograms_[i].getParameter<string>("variable") + "_filterEff"),int(histograms_[i].getParameter<int>("nBins")),histograms_[i].getParameter<double>("lBin"),histograms_[i].getParameter<double>("hBin"));
	// TH1F * h_pt = fs->make<TH1F>( "pt"  , "p_{t}", 100,  0., 100. );
	// TTree * h_ptt = fs->make<TTree>();
}

void AODTriggerAnalyzer::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup)
{
	edm::Handle< edm::TriggerResults > triggerBits;
	edm::Handle< BXVector<l1t::Muon> > l1Muons;
	edm::Handle< BXVector<l1t::EGamma> > l1EGammas;
	edm::Handle< reco::MuonCollection > recoMuons;
	edm::Handle< reco::PhotonCollection > recoPhotons;
	// edm::Handle< trigger::TriggerEvent > triggerSummary; 


	// edm::Handle<pat::TriggerObjectStandAloneCollection> triggerObjects;
	// edm::Handle<pat::PackedTriggerPrescales> triggerPrescales;

	iEvent.getByToken(triggerBits_, triggerBits);
	iEvent.getByToken(l1Muons_, l1Muons);
	iEvent.getByToken(l1EGammas_, l1EGammas);
	iEvent.getByToken(recoMuons_, recoMuons);
	iEvent.getByToken(recoPhotons_, recoPhotons);
	// iEvent.getByToken(triggerObjects_, triggerObjects);
	// iEvent.getByToken(triggerPrescales_, triggerPrescales);


	if (verbose) std::cout << "Configs: " << l1MuonOS_ << l1MuonIso_ << l1MuonQltMin_ << l1MuonQltMax_ << std::endl;
	for (std::vector<double>::const_iterator it = l1MuonPt_.begin(); it != l1MuonPt_.end(); it++ ){
		if (verbose) std::cout << *it << std::endl; 
	}



	// L1 Test
	bool l1Test = l1Filter(l1Muons, l1EGammas, iEvent);
	if (verbose) std::cout << "l1Test: " << l1Test << std::endl;

	// Define L3 Objects
	trigger::TriggerObjectCollection muonL3Objects = filterFinder(triggerSummaryLabel_, muonFilterTag_, iEvent);
	trigger::TriggerObjectCollection photonL3Objects = filterFinder(triggerSummaryLabel_, photonFilterTag_, iEvent);

	// HLT Test
	bool hltTest = hltFilter(muonL3Objects, photonL3Objects, iEvent);
	if (verbose) std::cout << "hltTest: " << hltTest << std::endl;

	// RECO Test
	bool recoTest = recoFilter(recoMuons, recoPhotons, iEvent);
	if (verbose) std::cout << "recoTest: " << recoTest << std::endl;

}

//find the filters
	trigger::TriggerObjectCollection 
AODTriggerAnalyzer::filterFinder(edm::EDGetTokenT<trigger::TriggerEvent> triggerSummaryLabel, edm::InputTag filterTag, const edm::Event &iEvent)
{

	edm::Handle<trigger::TriggerEvent> triggerSummary; 
	iEvent.getByToken(triggerSummaryLabel, triggerSummary);
	trigger::TriggerObjectCollection allTriggerObjects = triggerSummary->getObjects(); 
	//filterTag_ is the inputTag of the filter you want to match
	size_t filterIndex = (*triggerSummary).filterIndex(filterTag);
	trigger::TriggerObjectCollection filterObjects;
	if(filterIndex < (*triggerSummary).sizeFilters())
	{ 
		const trigger::Keys &keysObjects = (*triggerSummary).filterKeys(filterIndex);
		for(size_t j = 0; j < keysObjects.size(); j++)
		{
			trigger::TriggerObject foundObject = (allTriggerObjects)[keysObjects[j]];
			filterObjects.push_back(foundObject);
		}
	}
	if (verbose) std::cout<<filterObjects.size()<<endl; 
	return filterObjects;
}

	bool 
AODTriggerAnalyzer::hltFilter(trigger::TriggerObjectCollection muonL3Objects, trigger::TriggerObjectCollection photonL3Objects, const edm::Event &iEvent)
{
	// L3 Muons
	for (trigger::TriggerObjectCollection::const_iterator it = muonL3Objects.begin(); it != muonL3Objects.end(); it++) {
		if(it->pt() >= 0 ) {
			if (verbose) std::cout << "HLT Muon: " << it->pt() << std::endl;
		}
	}  

	// L3 Photons
	for (trigger::TriggerObjectCollection::const_iterator it = photonL3Objects.begin(); it != photonL3Objects.end(); it++) {
		if(it->pt() >= 0 ) {
			if (verbose) std::cout << "HLT Photon: " << it->pt() << std::endl;
		}
	} 
	return true;
}


	bool 
AODTriggerAnalyzer::recoFilter(edm::Handle< reco::MuonCollection > recoMuons, edm::Handle< reco::PhotonCollection > recoPhotons, const edm::Event &iEvent)
{

	int nDimuon=0, nJpsi=0, nPhoton=0;       
	std::vector<reco::Muon> myLeptons;
	std::vector<reco::Photon> myPhotons; 
	// Reco Muons
	for (reco::MuonCollection::const_iterator muon = recoMuons->begin(); muon != recoMuons->end(); muon++) {
		if (muon->isPFMuon()){
			if (muon->isTrackerMuon() || muon->isGlobalMuon()){
				if (verbose) std::cout << muon->charge() << std::endl;
				if (muon->pt()<minMuPt && std::abs(muon->eta())<maxMuEta){
					myLeptons.push_back(*muon);
					if(verbose) cout<<"Muon "<<muon->pt()<<endl;
				}  //eta and pt muon
			}  //muon type selection
		}  //PF muon
	}// Muon loop

	std::sort(myLeptons.begin(),myLeptons.end(), [](const reco::Muon &a, const reco::Muon &b){
			return a.pt() > b.pt();
			});

	if(verbose) std::cout<<" myLeptons.size() all  " << myLeptons.size() << std::endl;
	// bimuon selection
	if (myLeptons.size() == 2 && myLeptons.size() !=0) {
		//recoFilter_ = true;
		nDimuon++;
		std::cout<<"  Muons Multiplicity:  " << myLeptons.size() << std::endl; 
		std::cout<<" Dimuons Multiplicity:  " << nDimuon << std::endl;
		reco::Muon leadingMuon = myLeptons[0];
		reco::Muon trailingMuon = myLeptons[1];
		//Dimuons  selection
		if ((leadingMuon.charge() != trailingMuon.charge())) {
		} else {return false;}

		std::cout<< "Leading Muon pt, eta, phi, charge = " << leadingMuon.pt() << " "<< leadingMuon.eta() << " "<< leadingMuon.phi() << " " << leadingMuon.charge() << std::endl;
		std::cout<< "Trailing Muon  pt, eta, phi,charge = " << trailingMuon.pt() << " " << trailingMuon.eta() << " " << trailingMuon.phi() << " " << trailingMuon.charge()<< std::endl;

		//Invariant mass of dimuons
		double Mll = (leadingMuon.p4() + trailingMuon.p4()).mass();
		double MllpT = (leadingMuon.p4() + trailingMuon.p4()).pt();
		double Mlleta = (leadingMuon.p4() + trailingMuon.p4()).eta();
		double Mllphi = (leadingMuon.p4() + trailingMuon.p4()).phi();
		std::cout<< "Dimuons Invariant Mass Mll, pT, eta, phi: " << Mll << " " << MllpT << " " << Mlleta << " " << Mllphi << std::endl;
		if (leadingMuon.pt() > muonLeadPt || trailingMuon.pt() > muonTrailPt ) {

			// ***
			//   // jpsi peak
			//     // ***
			//
			if (Mll > minJPsiMass && Mll < maxJPsiMass){
				nJpsi++;                           
				std::cout<<" Invariant Mass in JPsi peak, pT, eta, phi " << Mll << " " << MllpT << " " << Mlleta << " " << Mllphi << std::endl;
				std::cout<<" Jpsi Multiplicity:  " <<  nJpsi << std::endl;
			}// jpsi selection
		}//lead and trail muon pT cut


		// Reco Photons
		for (reco::PhotonCollection::const_iterator photon = recoPhotons ->begin(); photon != recoPhotons->end(); photon++) {
			if(photon->pt() > GammaMinPtCut && photon->isPhoton()) {
				myPhotons.push_back(*photon);                       
				if(verbose) std::cout << "Reco Photon: " << photon->pt() << std::endl;
			}
		}

		std::sort(myPhotons.begin(),myPhotons.end(), [](const reco::Photon &a, const reco::Photon &b){
				return a.pt() > b.pt();
				});

		if (  myPhotons.size() == 1 && myPhotons.size() != 0 ){
			nPhoton++;
			std::cout<<" Photon Multiplicity:  " <<  nPhoton << std::endl;
			reco::Photon Gamma = myPhotons[0];		     
			DeltaR<reco::Muon, reco::Photon> deltaR;
			double drLeadMuPhoton = deltaR(leadingMuon,Gamma);
			double drTrailPhoton = deltaR(trailingMuon,Gamma);
			std::cout << " photon: pT, eta, phi " << Gamma.pt() << " "<< Gamma.eta() << " " << Gamma.phi() <<std::endl;                         std::cout<< " DeltaR(LeadMu,Photon) " << drLeadMuPhoton << " DeltaR(TrailMu,Photon) " << drTrailPhoton <<std::endl;
			if (drLeadMuPhoton > drLeadMuPhotonSel && drTrailPhoton > drTrailPhotonSel){

				double Mllg = (leadingMuon.p4() + trailingMuon.p4() + Gamma.p4()).mass();
				double MllgpT = (leadingMuon.p4() + trailingMuon.p4() + Gamma.p4()).pt();
				double Mllgeta = (leadingMuon.p4() + trailingMuon.p4() + Gamma.p4()).eta();
				double Mllgphi = (leadingMuon.p4() + trailingMuon.p4() + Gamma.p4()).phi();		 
				std::cout<< "Invariant Mass Mllg, pT, eta, phi: " << Mllg << " " << MllgpT << " " << Mllgeta << " " << Mllgphi << std::endl;	


			}// deltaR cuts	

		} else {return false;}//photon selection


	} else {return false;}//dimuons selection

	////////////////// 
	return true;
}//end RecoFilter

///////////////////////////////////////////////////////////////

	bool 
AODTriggerAnalyzer::l1Filter(edm::Handle< BXVector<l1t::Muon> > l1Muons, edm::Handle< BXVector<l1t::EGamma> > l1EGammas, const edm::Event &iEvent)
{
	// L1 Muons
	for (int ibx = l1Muons->getFirstBX(); ibx <= l1Muons->getLastBX(); ++ibx) {
		for (BXVector<l1t::Muon>::const_iterator it=l1Muons->begin(); it!=l1Muons->end(); it++){
			if (it->pt() >= 0){
				if (verbose) std::cout << "L1 Muon: " << it->pt() << std::endl;
				// l1upgrade_.muonEt .push_back(it->et());
				// l1upgrade_.muonEta.push_back(it->eta());
				// l1upgrade_.muonPhi.push_back(it->phi());
				// l1upgrade_.muonEtaAtVtx.push_back(l1t::MicroGMTConfiguration::calcMuonEtaExtra(*it));
				// l1upgrade_.muonPhiAtVtx.push_back(l1t::MicroGMTConfiguration::calcMuonPhiExtra(*it));
				// l1upgrade_.muonIEt .push_back(it->hwPt());
				// l1upgrade_.muonIEta.push_back(it->hwEta());
				// l1upgrade_.muonIPhi.push_back(it->hwPhi());
				// l1upgrade_.muonIDEta.push_back(it->hwDEtaExtra());
				// l1upgrade_.muonIDPhi.push_back(it->hwDPhiExtra());
				// l1upgrade_.muonChg.push_back(it->charge());
				// l1upgrade_.muonIso.push_back(it->hwIso());
				// l1upgrade_.muonQual.push_back(it->hwQual());
				// l1upgrade_.muonTfMuonIdx.push_back(it->tfMuonIndex());
				// l1upgrade_.muonBx .push_back(ibx);
				// l1upgrade_.nMuons++;
			}
		}
	}

	// L1 EGammas
	for (int ibx = l1EGammas->getFirstBX(); ibx <= l1EGammas->getLastBX(); ++ibx) {
		for (BXVector<l1t::EGamma>::const_iterator it=l1EGammas->begin(); it!=l1EGammas->end(); it++){
			if (it->pt() >= 0){
				if (verbose) std::cout << "L1 EGamma: " << it->pt() << std::endl;
				// l1upgrade_.egEt .push_back(it->pt());
				// l1upgrade_.egEta.push_back(it->eta());
				// l1upgrade_.egPhi.push_back(it->phi());
				// l1upgrade_.egIEt .push_back(it->hwPt());
				// l1upgrade_.egIEta.push_back(it->hwEta());
				// l1upgrade_.egIPhi.push_back(it->hwPhi());
				// l1upgrade_.egIso.push_back(it->hwIso());
				// l1upgrade_.egBx .push_back(ibx);
				// l1upgrade_.egTowerIPhi.push_back(it->towerIPhi());
				// l1upgrade_.egTowerIEta.push_back(it->towerIEta());
				// l1upgrade_.egRawEt.push_back(it->rawEt());
				// l1upgrade_.egIsoEt.push_back(it->isoEt());
				// l1upgrade_.egFootprintEt.push_back(it->footprintEt());
				// l1upgrade_.egNTT.push_back(it->nTT());
				// l1upgrade_.egShape.push_back(it->shape());
				// l1upgrade_.egTowerHoE.push_back(it->towerHoE());
				// l1upgrade_.nEGs++;
			}
		}
	}
	return true;
}



//define this as a plug-in
DEFINE_FWK_MODULE(AODTriggerAnalyzer);

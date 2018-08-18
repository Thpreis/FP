// Usage:
// .L AccumulatedAsymmetrie.C
// Asymmetrie(xmin, xmax, "fp13ohneB.root", "fp13mitB.root");

#include <TFile.h>
#include <TROOT.h>
#include <TH1D.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TMath.h>
#include <TColor.h>
#include <TStyle.h>

#include <iostream>
#include <string>
#include <cmath>

using namespace std;

////////////////////////////////////////////////////////////////////////
// Hifsfunktionen
////////////////////////////////////////////////////////////////////////
// zuerst diejenigen, die mit dem Fit zu tun haben
double fitFuncAsymmetrie(double *x, double *par)
{
	// par[0] - background
	// par[1] - P * A
	// par[2] - muon Lamor frequency in (ns)^(-1)
	// par[3] - phase
	return par[0] + par[1]/2.0 * TMath::Cos(x[0] * par[2] + par[3]);
}

TF1* getFitFunction()
{
	TF1 *func = new TF1("fitFuncAsymmetrie", fitFuncAsymmetrie,
			0., 2e4, 4);
	func->SetParameters(0.01, 0.05, 3.4e-3, 0.0);
	func->SetParName(0, "BG");
	func->SetParName(1, "P*A");
	func->SetParName(2, "#omega_{L}");
	func->SetParName(3, "#phi_{0}");
	func->SetParLimits(0, -10., 10.);
	func->SetParLimits(1, 0., 1.);
	func->SetParLimits(2, 5e-4, 5e-2);
	func->SetParLimits(3, -TMath::Pi(), 2.0 * TMath::Pi());
	func->SetNpx(200);
	func->SetLineColor(kRed);

	return func;
}

void fitAsymmetrie(TH1D *h, double xmin, double xmax)
{
	TF1 *fitFunc = getFitFunction();
	h->Fit(fitFunc, "", "", xmin, xmax);
	delete fitFunc;
}

////////////////////////////////////////////////////////////////////////
// die "Hauptfunktion", in der all die kleinen Helfer genutzt werden ;)
////////////////////////////////////////////////////////////////////////
void Asymmetrie(double xmin = 3e2, double xmax = 2e4)
{
	////////////////////////////////////////////////////////////////
	// Einstellungen fuer graphische Darstellung der Plots setzen
	////////////////////////////////////////////////////////////////
	gStyle->SetOptFit(1111);
	gStyle->SetOptStat("ni");
	gStyle->SetStatFormat("g");
	gStyle->SetMarkerStyle(20);
	gStyle->SetMarkerSize(0.5);
	gStyle->SetHistLineWidth(1);
	gROOT->ForceStyle();

	////////////////////////////////////////////////////////////////
	// Datenfiles oeffnen und Histogramme einlesen
	////////////////////////////////////////////////////////////////
	TFile *fmy = new TFile("Results.root", "READ");
	TFile *fall = new TFile("ResultsAll.root", "READ");

	// Histogramme mit B-Feld haben ein 'm' im Namen, diejenigen
	// ohne nicht
	// eigene Daten laden
	TH1D *ho = (TH1D*) fmy->Get("ho");
	TH1D *hu = (TH1D*) fmy->Get("hu");
	TH1D *hmo = (TH1D*) fmy->Get("hmo");
	TH1D *hmu = (TH1D*) fmy->Get("hmu");
	// Daten anderer Gruppen laden
	TH1D *hoAll = (TH1D*) fall->Get("hoAll");
	TH1D *huAll = (TH1D*) fall->Get("huAll");
	TH1D *hmoAll = (TH1D*) fall->Get("hmoAll");
	TH1D *hmuAll = (TH1D*) fall->Get("hmuAll");

	// kombinieren
	hoAll->Add(ho);
	huAll->Add(hu);
	hmoAll->Add(hmo);
	hmuAll->Add(hmu);

	////////////////////////////////////////////////////////////////
	// Summen und Differenzen zwischen Zerfaellen mit und ohne
	// Magnetfeld berechnen
	////////////////////////////////////////////////////////////////
	// Histogramme  buchen und mit einer Kopie fuellen
	TH1D *osum = (TH1D*) hmoAll->Clone(), *odiff = (TH1D*) hmoAll->Clone();
	TH1D *usum = (TH1D*) hmuAll->Clone(), *udiff = (TH1D*) hmuAll->Clone();
	// Skalierungsfaktoren berechnen,
	double scaleO = hmoAll->Integral() / hoAll->Integral();
	double scaleU = hmuAll->Integral() / huAll->Integral();
	// Summen/Differenzen bilden
	osum->Add(hoAll, scaleO);
	odiff->Add(hoAll, -scaleO);
	usum->Add(huAll, scaleU);
	udiff->Add(huAll, -scaleU);

	// vernuenftige Namen und Titel vergeben
	TH1D *asymOAll = (TH1D*) odiff->Clone();
	TH1D *asymUAll = (TH1D*) udiff->Clone();
	asymOAll->SetNameTitle("asymOAll", "Alle Gruppen: Asymmetrie im Zerfall nach oben");
	asymUAll->SetNameTitle("asymUAll", "Alle Gruppen: Asymmetrie im Zerfall nach unten");
	// und teilen
	asymOAll->Divide(osum);
	asymUAll->Divide(usum);

	// oben/unten kombinieren
        TH1D *diff = (TH1D*) udiff->Clone();
        TH1D *sum  = (TH1D*) usum->Clone();
	// Ist Ihnen klar, warum in der naechsten Zeile abgezogen wird?
	// Ueberlegen Sie sich dazu, positive/negative Myonen eine Vor-
	// zugszerfallsrichtung haben, bevor sie ins Magnetfeld kommen.
        diff->Add(odiff,-1);
        sum->Add(osum,1);
	TH1D *asymAll = (TH1D*) diff->Clone();
	asymAll->SetNameTitle("asymAll", "Alle Gruppen: Asymmetrie im Zerfall");
	asymAll->Divide(sum);

	// Jetzt wird gefittet
	cout << endl << string(72, '*') << endl << asymOAll->GetTitle() <<
		endl << string(72, '*') << endl;
	fitAsymmetrie(asymOAll, xmin, xmax);

	cout << endl << string(72, '*') << endl << asymUAll->GetTitle() <<
		endl << string(72, '*') << endl;
	fitAsymmetrie(asymUAll, xmin, xmax);

	cout << endl << string(72, '*') << endl << asymAll->GetTitle() <<
		endl << string(72, '*') << endl;
	fitAsymmetrie(asymAll, xmin, xmax);

	// Histogramme zeichnen
	TCanvas *c1 = new TCanvas("canvas1", asymOAll->GetTitle());
	c1->cd();
	c1->Clear();
	asymOAll->Draw("E");
	asymOAll->SetAxisRange(0.0, 1.25 * xmax, "X");
	asymOAll->SetAxisRange(-0.4, 0.4, "Y");

	TCanvas *c2 = new TCanvas("canvas2", asymUAll->GetTitle());
	c2->cd();
	c2->Clear();
	asymUAll->Draw("E");
	asymUAll->SetAxisRange(0.0, 1.25 * xmax, "X");
	asymUAll->SetAxisRange(-0.4, 0.4, "Y");

	TCanvas *c3 = new TCanvas("canvas3", asymAll->GetTitle());
	c3->cd();
	c3->Clear();
	asymAll->Draw("E");
	asymAll->SetAxisRange(0.0, 1.25 * xmax, "X");
	asymAll->SetAxisRange(-0.4, 0.4, "Y");
}

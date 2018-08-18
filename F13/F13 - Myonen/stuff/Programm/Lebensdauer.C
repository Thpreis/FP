// Usage:
// .L Lebensdauer.C
// Lebensdauer(xmin, xmax, "fp13.root")

#include <TH1D.h>
#include <TF1.h>
#include <TFile.h>
#include <TString.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TAxis.h>
#include <TArrayD.h>
#include <TMath.h>
#include <TColor.h>
#include <TROOT.h>
#include <iostream>
#include <cmath>

using namespace std;

// Hilfsfunktionen
double getAfterpulseScaleFactor(int scintNr, bool up, TH1D* h8)
{
	// FIXME: im folgenden Array werden die Skalierungsfaktoren fuer
	// Zerfaelle nach oben/nach unten eingetragen
	// alternativ koennen Sie auch Code schreiben, der die
	// benoetigten Informationen direkt aus h8 ausliest
	static const double scaleFactorUp[] = {
		0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0
	};
	static const double scaleFactorDown[] = {
		0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0
	};
	if (up) {
		return scaleFactorUp[scintNr];
	} else {
		return scaleFactorDown[scintNr];
	}
}

// die Fitfunktion selbst bekommt eine Koordinate x[0] und einen Satz von
// Parametern (im Array par) und gibt einen reellen Wert zurueck
double fitFuncF(double *x, double *par)
{
	// par[0] background level
	// par[1] number of muons
	// par[2] muon lifetime
	return par[1] * (TMath::Exp(-x[0] / par[2])) + par[0];
}

// ROOT fittet ein ein Funktionsobjekt vom Typ TF1, das in der dieser
// Funktion konfiguriert wird
TF1* setFitFunction()
{
	// TF1(name, funktion, xmin, xmax, npar)
	TF1 *fitFunc = new TF1("MuonLifetime",
			fitFuncF, 0., 20000., 3);
	// Parameter mit Namen versehen
	fitFunc->SetParName(0, "BG");
	fitFunc->SetParName(1, "# #mu");
	fitFunc->SetParName(2, "#tau_{0}");
	// Funktion mit 200 Punkten zeichnen
	fitFunc->SetNpx(200);
	// Grenzen fuer die Parameter setzen
	fitFunc->SetParLimits(0, 0.01, 100000.);
	fitFunc->SetParLimits(1, 0., 100000.);
	fitFunc->SetParLimits(2, 1000., 10000.);
	// Startwerte fuer Parameter setzen
	fitFunc->SetParameters(10., 5000., 2000.);
	// Farbe setzen
	fitFunc->SetLineColor(kRed);

	return fitFunc;
}

// diese Funktion macht die eigentliche Arbeit
void Lebensdauer(double xmin = 300., double xmax = 20000.,
		const char *filename = "fp13.root")
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
	// ROOT file oeffnen
	////////////////////////////////////////////////////////////////
	TFile *f = new TFile(filename, "READ");

	////////////////////////////////////////////////////////////////
	// Histogramme einlesen
	////////////////////////////////////////////////////////////////
	TH1D *h8 = (TH1D*)f->Get("h8");  

	TH1D *a[8], *b[8], *x[8];
	for (int i = 0; i < 8; ++i) {
		a[i] = (TH1D*) f->Get(Form("a%d", i));
		b[i] = (TH1D*) f->Get(Form("b%d", i));
		x[i] = (TH1D*) f->Get(Form("x%d", i));
		a[i]->Sumw2();
		b[i]->Sumw2();
		x[i]->Sumw2();
	}

	////////////////////////////////////////////////////////////////
	// Nachpulsspektren abziehen
	////////////////////////////////////////////////////////////////
	// FIXME: tragen Sie oben in getAfterpulseScaleFactor die
	// von Ihnen berechneten Skalierungsfaktoren ein
	for (int iLayer = 1; iLayer <= 6; ++iLayer) {
		// die skalierten Nachpulsspektren werden hier
		// von den gemessenen Zerfallsspektren abgezogen
		a[iLayer]->Add(x[iLayer],
				- getAfterpulseScaleFactor(iLayer, true, h8));
		b[iLayer]->Add(x[iLayer],
				- getAfterpulseScaleFactor(iLayer, false, h8));
	}

	////////////////////////////////////////////////////////////////
	// Histogramme aus den verschiedenen Szintillatoren kombinieren:
	// erst Zerfaelle nach unten/oben separat, dann beide zusammen
	////////////////////////////////////////////////////////////////
	
	// Wir buchen zwei Histogramme mit den selben "Abmessungen" wie
	// a[2], und setzten diese zurueck. Dann addieren wir die
	// Szintillatoren, die wir in unserer Messung haben wollen
	TH1D *ho = new TH1D(*a[2]);
	ho->Reset();
	ho->SetNameTitle("ho", "Zerfall nach oben");

	TH1D *hu = new TH1D(*b[2]);
	hu->Reset();
	hu->SetNameTitle("hu", "Zerfall nach unten");

	ho->Add(a[1], 1.0);	// Zerfaelle nach oben
	ho->Add(a[2], 1.0);
	ho->Add(a[3], 1.0);
	ho->Add(a[4], 1.0);
	ho->Add(a[5], 1.0);
	ho->Add(a[6], 1.0);

	hu->Add(b[2], 1.0);	// Zerfaelle nach unten
	hu->Add(b[3], 1.0);
	hu->Add(b[4], 1.0);
	hu->Add(b[5], 1.0);
	hu->Add(b[6], 1.0);

	// jetzt Zerfaelle nach oben/unten in ein gemeinsames Histogramm
	// kombinieren
	TH1D *hL = new TH1D(*ho);
	hL->Add(hu);
	hL->SetNameTitle("hL", "Lebensdauer");

	////////////////////////////////////////////////////////////////
	// Fitfunktionen definieren und Startwerte setzen
	////////////////////////////////////////////////////////////////
	// das Verfahren ist etwas laenglich und bekommt zur besseren
	// Lesbarkeit seine eigene Funktion definiert
	TF1 *fitFunc = setFitFunction();

	////////////////////////////////////////////////////////////////
	// Lebensdauer fitten
	////////////////////////////////////////////////////////////////
	// eine nette Meldung ausgeben...
	cout << endl << endl << string(72, '*') << endl <<
		"FIT: Lebensdauer - " << ho->GetTitle() << endl <<
		string(72, '*') << endl << endl;
	// ... und fitten
	ho->Fit(fitFunc, "", "", xmin, xmax);

	// das selbe fuer die Zerfaelle nach unten
	cout << endl << endl << string(72, '*') << endl <<
		"FIT: Lebensdauer - " << hu->GetTitle() << endl <<
		string(72, '*') << endl << endl;
	hu->Fit(fitFunc, "", "", xmin, xmax);

	// und dann nochmal fuer beide kombiniert
	cout << endl << endl << string(72, '*') << endl <<
		"FIT: Lebensdauer - " << hL->GetTitle() << endl <<
		string(72, '*') << endl << endl;
	hL->Fit(fitFunc, "", "", xmin, xmax);

	////////////////////////////////////////////////////////////////
	// ganz viele Histogramme zeichnen
	////////////////////////////////////////////////////////////////
	// Ein Canvas ist eine "Leinwand", auf die geplottet wird
	TCanvas *c0 = new TCanvas("c0", "Zerfaelle nach oben");
	c0->cd(); 		// Canvas ansteuern
	c0->Clear();		// und leeren
	c0->Divide(2, 3);	// in sechs Felder aufteilen
	gPad->SetLogy();	// in y logarithmische Skala setzen
	for (int i = 1; i < 7; ++i) {
		c0->cd(i);	// ein Feld ansteuern
		gPad->SetLogy();// in y logarithmische Skala setzen
		a[i]->DrawClone("e"); // und zeichnen
	}

	TCanvas *c1 = new TCanvas("c1", "Zerfaelle nach unten");
	c1->cd();
	c1->Clear();
	c1->Divide(2,3);
	gPad->SetLogy();
	for (int i = 1; i < 7; ++i) {
		c1->cd(i);
		gPad->SetLogy();
		b[1 + i]->DrawClone("e");
	}

	TCanvas *c2 = new TCanvas("c2", "Nachpulse");
	c2->cd();
	c2->Clear();
	c2->Divide(2, 3);
	gPad->SetLogy();

	for (int i = 1; i < 7; ++i) {
		c2->cd(i);
		gPad->SetLogy();
		x[i]->DrawClone("e");
	}

	TCanvas *c3 = new TCanvas("c3", "Lebensdauer nach oben/unten");
	c3->cd();
	c3->Clear();
	c3->Divide(1, 2);
	gPad->SetLogy();

	c3->cd(1);
	gPad->SetLogy();
	ho->DrawClone("e");

	c3->cd(2);
	gPad->SetLogy();
	hu->DrawClone("e");

	TCanvas *c4 = new TCanvas("c4", "Lebensdauer kombiniert");
	c4->cd();
	c4->Clear();
	gPad->SetLogy();
	hL->DrawClone("e");
}

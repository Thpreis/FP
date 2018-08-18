// Usage:
// .L Asymmetrie2.C
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

// Anzahl der benutzten Detektorlagen
const int nLayers = 6;

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

// Hilfsfunktion zur Rueckgabe der korrekten Skalierungsfaktoren fuer
// Nachpulse
// FIXME: Tragen Sie in dieser Funktion Ihre berechneten Skalierungs-
// faktoren ein. Bedenken Sie, dass fuer die Messungen mit Magnet-
// feld die Faktoren neu berechnet werden muessen! (warum?)
double getAfterpulseScaleFactor(int scintNr, bool up, TH1D* h8)
{
	// die Konvention ist, negative Szintillatornummern zu uebergeben,
	// wenn die Skalierungsfaktoren _mit_ B-Feld zurueckgegeben werden
	// sollen
	bool withBField = (scintNr < 0);
	// wir wissen jetzt, welche Faktoren benoetigt werden, und brauchen
	// eigentlich positive Szintillatornummern, daher nehmen wir den
	// Betrag
	scintNr = abs(scintNr);
	// FIXME: im folgenden Array werden die Skalierungsfaktoren fuer
	// Zerfaelle nach oben/nach unten eingetragen, einmal mit, einmal
	// ohne B-Feld
	// alternativ koennen Sie auch Code schreiben, der die
	// benoetigten Informationen direkt aus h8 ausliest
	// der Code, der diese Funktion aufruft, uebergibt das jeweils
	// "richtige" h8
	static const double scaleFactorUpBoff[] = {
		0, 0, 0, 0,
		0, 0, 0, 0
	};
	static const double scaleFactorDownBoff[] = {
		0, 0, 0, 0,
		0, 0, 0, 0
	};
	static const double scaleFactorUpBon[] = {
		0, 0, 0, 0,
		0, 0, 0, 0
	};
	static const double scaleFactorDownBon[] = {
		0, 0, 0, 0,
		0, 0, 0, 0
	};
	if (withBField) {
		if (up) return scaleFactorUpBon[scintNr];
		else return scaleFactorDownBon[scintNr];
	} else {
		if (up) return scaleFactorUpBoff[scintNr];
		else return scaleFactorDownBoff[scintNr];
	}
}

////////////////////////////////////////////////////////////////////////
// die "Hauptfunktion", in der all die kleinen Helfer genutzt werden ;)
////////////////////////////////////////////////////////////////////////
void Asymmetrie(double xmin = 3e2, double xmax = 2e4,
		const char *ohneBfile = "fp13.root",
		const char *mitBfile = "fp13mitB.root")
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
	// Resultate werden automatisch auch in der Datei Results.root
	// gespeichert
	////////////////////////////////////////////////////////////////
	TFile *fmb = new TFile(mitBfile, "READ");
	TFile *fob = new TFile(ohneBfile, "READ");
	TFile *fout = new TFile("Results.root", "RECREATE");

	// Histogramme mit B-Feld haben ein 'm' im Namen, diejenigen
	// ohne nicht
	TH1D *a[nLayers], *b[nLayers], *x[nLayers], *am[nLayers], *bm[nLayers], *xm[nLayers];
	TH1D *h8 = (TH1D*) fob->Get("h8");
	TH1D *hm8 = (TH1D*) fmb->Get("h8");
	for (int i = 0; i < nLayers; ++i) {
		a[i] = (TH1D*) fob->Get(Form("a%d", i));
		b[i] = (TH1D*) fob->Get(Form("b%d", i));
		x[i] = (TH1D*) fob->Get(Form("x%d", i));
		am[i] = (TH1D*) fmb->Get(Form("a%d", i));
		bm[i] = (TH1D*) fmb->Get(Form("b%d", i));
		xm[i] = (TH1D*) fmb->Get(Form("x%d", i));
		a[i]->Sumw2(); b[i]->Sumw2(); x[i]->Sumw2();
		am[i]->Sumw2(); bm[i]->Sumw2(); xm[i]->Sumw2();
	}

	////////////////////////////////////////////////////////////////
	// Nachpulskorrektur
	////////////////////////////////////////////////////////////////
	for (int i = 1; i < nLayers-1; ++i) {
		// ohne B-Feld
		a[i]->Add(x[i], - getAfterpulseScaleFactor(i, true, h8));
		b[i]->Add(x[i], - getAfterpulseScaleFactor(i, false, h8));
		// das selbe nocheinmal mit B-Feld
		// da sich die Korrekturfaktoren fuer die Messung mit
		// und ohne B-Feld unterscheiden werden, werden negative
		// Szintillatornummern als Zeichen dafuer ubergeben, dass
		// die Korrekturfaktoren mit B-Feld gewuenscht werden
		am[i]->Add(xm[i], - getAfterpulseScaleFactor(-i, true, hm8));
		bm[i]->Add(xm[i], - getAfterpulseScaleFactor(-i, false, hm8));
	}

	////////////////////////////////////////////////////////////////
	// Zerfaelle nach oben/unten kombinieren
	////////////////////////////////////////////////////////////////
	// kopiere die Abmessungen durch Kopieren des ganzen Histogramms
	TH1D *ho = (TH1D*) a[2]->Clone(), *hu = (TH1D*) a[2]->Clone();
	TH1D *hmo = (TH1D*) a[2]->Clone(), *hmu = (TH1D*) a[2]->Clone();
	// anschliessend werden die Daten des Histogramms zurueckgesetzt
	ho->Reset(); hu->Reset(); hmo->Reset(); hmu->Reset();
	// Histogramme mit Namen versehen
	ho->SetNameTitle("ho", "Zerfall nach oben ohne B-Feld");
	hu->SetNameTitle("hu", "Zerfall nach unten ohne B-Feld");
	hmo->SetNameTitle("hmo", "Zerfall nach oben mit B-Feld");
	hmu->SetNameTitle("hmu", "Zerfall nach unten mit B-Feld");

	// Szintillatoren auswaehlen: zuerst ohne B-Feld
	ho->Add(a[1], 1.0);
	ho->Add(a[2], 1.0);	hu->Add(b[2], 1.0);
	ho->Add(a[3], 1.0);	hu->Add(b[3], 1.0);
	ho->Add(a[4], 1.0);	hu->Add(b[4], 1.0);
	//ho->Add(a[5], 1.0);	hu->Add(b[5], 1.0);
	//ho->Add(a[6], 1.0);	hu->Add(b[6], 1.0);
	// dann mit B-Feld
	hmo->Add(am[1], 1.0);
	hmo->Add(am[2], 1.0);	hmu->Add(bm[2], 1.0);
	hmo->Add(am[3], 1.0);	hmu->Add(bm[3], 1.0);
	hmo->Add(am[4], 1.0);	hmu->Add(bm[4], 1.0);
	//hmo->Add(am[5], 1.0);	hmu->Add(bm[5], 1.0);
	//hmo->Add(am[6], 1.0);	hmu->Add(bm[6], 1.0);
	// Resultate speichern
	fout->WriteTObject(ho);
	fout->WriteTObject(hu);
	fout->WriteTObject(hmo);
	fout->WriteTObject(hmu);

	////////////////////////////////////////////////////////////////
	// Summen und Differenzen zwischen Zerfaellen mit und ohne
	// Magnetfeld berechnen
	////////////////////////////////////////////////////////////////
	// Histogramme  buchen und mit einer Kopie fuellen
	TH1D *osum = (TH1D*) hmo->Clone(), *odiff = (TH1D*) hmo->Clone();
	TH1D *usum = (TH1D*) hmu->Clone(), *udiff = (TH1D*) hmu->Clone();
	// Skalierungsfaktoren berechnen,
	double scaleO = hmo->Integral() / ho->Integral();
	double scaleU = hmu->Integral() / hu->Integral();
	// Summen/Differenzen bilden
	osum->Add(ho, scaleO);
	odiff->Add(ho, -scaleO);
	usum->Add(hu, scaleU);
	udiff->Add(hu, -scaleU);

	// vernuenftige Namen und Titel vergeben
	TH1D *asymO = (TH1D*) odiff->Clone();
	TH1D *asymU = (TH1D*) udiff->Clone();
	asymO->SetNameTitle("asymO", "Asymmetrie im Zerfall nach oben");
	asymU->SetNameTitle("asymU", "Asymmetrie im Zerfall nach unten");
	// und teilen
	asymO->Divide(osum);
	asymU->Divide(usum);

	// oben/unten kombinieren
        TH1D *diff = (TH1D*) udiff->Clone();
        TH1D *sum  = (TH1D*) usum->Clone();
	// Ist Ihnen klar, warum in der naechsten Zeile abgezogen wird?
	// Ueberlegen Sie sich dazu, positive/negative Myonen eine Vor-
	// zugszerfallsrichtung haben, bevor sie ins Magnetfeld kommen.
        diff->Add(odiff,-1);
        sum->Add(osum,1);
	TH1D *asym = (TH1D*) diff->Clone();
	asym->SetNameTitle("asym", "Asymmetrie im Zerfall");
	asym->Divide(sum);

	// Jetzt wird gefittet
	cout << endl << string(72, '*') << endl << asymO->GetTitle() <<
		endl << string(72, '*') << endl;
	fitAsymmetrie(asymO, xmin, xmax);

	cout << endl << string(72, '*') << endl << asymU->GetTitle() <<
		endl << string(72, '*') << endl;
	fitAsymmetrie(asymU, xmin, xmax);

	cout << endl << string(72, '*') << endl << asym->GetTitle() <<
		endl << string(72, '*') << endl;
	fitAsymmetrie(asym, xmin, xmax);

	// Resultat speichern
	fout->WriteTObject(asymO);
	fout->WriteTObject(asymU);
	fout->WriteTObject(asym);

	// Histogramme zeichnen
	TCanvas *c1 = new TCanvas("canvas1", asymO->GetTitle());
	c1->cd();
	c1->Clear();
	asymO->Draw("E");
	asymO->SetAxisRange(0.0, 1.25 * xmax, "X");
	asymO->SetAxisRange(-0.4, 0.4, "Y");

	TCanvas *c2 = new TCanvas("canvas2", asymU->GetTitle());
	c2->cd();
	c2->Clear();
	asymU->Draw("E");
	asymU->SetAxisRange(0.0, 1.25 * xmax, "X");
	asymU->SetAxisRange(-0.4, 0.4, "Y");

	TCanvas *c3 = new TCanvas("canvas3", asym->GetTitle());
	c3->cd();
	c3->Clear();
	asym->Draw("E");
	asym->SetAxisRange(0.0, 1.25 * xmax, "X");
	asym->SetAxisRange(-0.4, 0.4, "Y");
}

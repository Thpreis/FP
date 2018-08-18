////////////////////////////////////////////////////////////////////////
// FP13 -- Lebensdauer von Myonen
//
// Klasse zur Datenanalyse im Rahmen von FP13
//
// v00:	Wed Apr 26 12:22:13 CEST 2006
//
// 	basierend auf Code von Sarah Dambach
//
// 		Johannes Albrecht <albrecht@physi.uni-heidelberg.de>
//		Stefan Schenk <schenk@physi.uni-heidelberg.de>
//
// v01: Wed Sep 26 2007
// 	Aenderungen aufgrund der Aufraeumarbeiten in fp13Analysis.cc
//	 	Manuel Schiller <schiller@physi.uni-heidelberg.de>
//
// v02: Tue Mar 25 2008
// 	Kleinere Aenderungen fuer korrektere Nachpulsbestimmung
//	 	Manuel Schiller <schiller@physi.uni-heidelberg.de>
////////////////////////////////////////////////////////////////////////

#ifndef FP13ANALYSIS_H
#define FP13ANALYSIS_H

// C++ headers
#include <iostream>
#include <vector>
#include <string>
// ROOT headers
#include <TFile.h>
#include <TH1.h>

// Namen aus dem Namensraum std verfuegbar machen
using namespace std;

// Geruest des Analyse-Objekts deklarieren
class fp13Analysis 
{
protected:
	// Klassenvariablen - sie sind "protected", d.h. nur die Klasse selbst
	// und deren Kinder koennen auf diese Variablen zugreifen

	// Anzahl der Detektorlagen
	static const int nLayers = 6;
	// Minimale Verzoegerung zwischen Zeitbins, damit Ergebnisse
	// beruecksichtigt werden (in ns)
	// (Detektor/Auslesekette hat Totzeit! 55 ns scheinen sinnvoll,
	// entsprechen etwa der doppelten Pulsbreite nach Diskriminator)
	static const int minDelay = 55;

	// Ereigniszaehler
	unsigned long eventCounter;
	// wie viele Ereignisse wurden tatsaechlich analysiert
	unsigned long analyzedCounter;

	// Letzte Detektorlage, durch die das einlaufende Myon
	// von oben kontinuierlich durchgelaufen ist
	// wird beim Analysieren ausgefuellt
	int lastMuonLayer;

	// Vektoren, die fuer jedes Zeitbin mit Hits einen Eintrag enthalten:
	// Bitmasken der Detektorlagen, die anzeigen, welche Lage gefeuert hat
	vector<int> detectorHitMask;
	// Zeiten zu den registrierten Detektorhits
	vector<int> detectorHitTimes; 

	// Input Datenfile
	istream& inputFile; 
	// Output Histogrammfile (spezieller Datentyp fuer ROOT-Files)
	TFile& outputFile;

	// Namen von Ein- und Ausgabedatei
	string inputFileName, outputFileName;
	
	// Benoetigte Histogramme, die abgespeichert werden
	// Anzahl der Hits in den Detekorlagen
	TH1D *h1;
	// Anzahl der Nachpulse in den Detektorlagen aus durchgehenden
	// und stoppenden Myonen
	TH1D *h2;
	// Anzahl der Zerfaelle nach oben in den Detektorlagen
	TH1D *h3;
	// Anzahl der Zerfaelle nach unten in den Detektorlagen
	TH1D *h4;
	// Anzahl der Nachpulse in den Detektorlagen aus durchgehenden
	// Myonen
	TH1D *h5;
	// Anzahl der Hits in den Zeitfenstern
	TH1D *h6;
	// Anzahl der Hits in den beruecksichtitgen Zeitfenstern
	TH1D *h7;
	// Anzahl der Hits des einlaufenden Muons in den Detektorlagen
	TH1D *h8;
	
	// Vektoren von Histogrammen -- fuer jede Detektorlage eines
	// Zeit des ersten Hits
	vector<TH1D *> h21;
	// Zeit der Nachpulse (aus der verbesserten Analysemethode)
	vector<TH1D *> h22;
	// Zeit der Zerfaelle nach oben
	vector<TH1D *> h23;
	// Zeit der Zerfaelle nach unten
	vector<TH1D *> h24;
	// Zeit der Nachpulse aus durchgehenden Myonen (aus der
	// vereinfachten Nachpulsanalyse)
	vector<TH1D *> h25;

	// Datentyp definieren: Aufzaehlung der moeglichen Eventtypen fuer
	// Flags
	typedef enum {
		FlagNone = 0,
		FlagDecayDown = 1,
		FlagDecayUp = 2,
		FlagDecay = FlagDecayDown | FlagDecayUp,
		FlagAfterpulseSimple = 4,
		FlagAfterpulseImproved = 8,
		FlagAfterpulse = FlagAfterpulseSimple | FlagAfterpulseImproved
	} EventFlags;
	// Variable mit Flags fuer Event
	// (Datentyp: EventFlags, Variablenname: eventFlags (!))
	EventFlags eventFlags;

public:
	// Oeffentlich verfuegbare Methode (koennen von ausserhalb des
	// Objektes angesprochen werden)
	
	// Konstruktor - initialisiert ein neues Analyseobjekt
	fp13Analysis(const string& inputFileName,
			const string& outputFileName);
	
	// Destruktor - erledigt die Aufraeumarbeiten, wenn das Objekt nicht
	// mehr gebraucht wird
	virtual ~fp13Analysis(); 

	// Analysefunktionen, die extern angesprochen werden koennen
	
	// Einlesen eines Ereignisses
	// gibt im Erfolgsfall 0 zurueck
	virtual int readEvent();

	// Durchfuehrung der Analyse eines Ereignisses
	// Aufgaben der Methode:
	// - Erkennen von Myonen, die im Detektor stoppen und dann nach oben
	//   oder unten zerfallen
	// - Erkennen von Nachpulsen
	// - Buchfuehrung ueber die gesammelten Daten, d.h. Fuellen
	//   entsprechender Histogramme
	virtual void analyze(); 

	unsigned long getNoOfEvents();
	unsigned long getNoOfAnalyzedEvents();
	
protected:
	// "protected" Hilfsfunktionen, die nur von Methoden innerhalb der
	// Klasse (und deren Kindern, daher nicht private sondern protected)
	// aufgerufen werden koennen
	
	// Buchen der Histogramme
	// wird im Konstruktor aufgerufen, daher nicht virtuell
	void bookHistograms();

	// Bestimmung der letzten Detektorlage, die vom einlaufenden Myon
	// beim kontinuierlichen Durchlaufen des Detektors getroffen wurde
	// setzt lastMuonLayer (s. o.)
	virtual int determineLastLayerHitByIncomingMuon();

	// Finden eines Muonzerfalls nach oben
	// gibt Lage des Zerfallselektrons/positrons zurueck
	virtual int findDecayUpward(int timeBin);

	// Finden eines Muonzerfalls nach unten
	// gibt Lage des Zerfallselektrons/positrons zurueck
	virtual int findDecayDownward(int timeBin);

	// Finden von Nachpulsen anhand durchgehender Myonen
	// arbeitet sich nach oben, startend bei Lager startLayer
	// gibt Lage des Nachpulses zurueck (oder -1, falls nichts gefunden)
	// um mehrere Nachpulse im selben Zeitbin zu finden, einfach
	// mit vorherigem Rueckgabewert nochmal aufrufen
	// falls fuer startLayer der Wert -1 uebergeben wird, nimmt die
	// Routine an, dass in einem neuen Zeitbin gesucht wird und setzt
	// startLayer auf einen sinnvollen Anfangswer
	virtual int findAfterpulsesUsingThroughGoingMuons(int timeBin, int startLayer);

	// Verbesserte Nachpulsanalyse
	// arbeitet sich nach oben, startend bei Lager startLayer
	// gibt Lage des Nachpulses zurueck (oder -1, falls nichts gefunden)
	// um mehrere Nachpulse im selben Zeitbin zu finden, einfach
	// mit vorherigem Rueckgabewert nochmal aufrufen
	// falls fuer startLayer der Wert -1 uebergeben wird, nimmt die
	// Routine an, dass in einem neuen Zeitbin gesucht wird und setzt
	// startLayer auf einen sinnvollen Anfangswer
	virtual int findAfterpulsesImproved(int timeBin, int startLayer);
};

#endif

// Dateiende

////////////////////////////////////////////////////////////////////////
// FP13 -- Lebensdauer von Myonen
//
// Klasse zur Datenanalyse im Rahmen von FP13
//
//
// v00	Wed Apr 26 12:22:13 CEST 2006
// v01
//	basierend auf Code von Sarah Dambach
//	aktuelle Verantwortliche:
//		Johannes Albrecht <albrecht@physi.uni-heidelberg.de>
//		Stefan Schenk <schenk@physi.uni-heidelberg.de>
// v02	Tue Sep 25 2007
// 	Ueberarbeitung des Codes zur besseren Lesbarkeit und
// 	effektiveren Fehlerbehandlung
// 		Manuel Schiller <schiller@physi.uni-heidelberg.de>
// v03	Thu Dec 06 2007
// 	strengere Validierung der Eingabedaten
// 		Manuel Schiller <schiller@physi.uni-heidelberg.de>
// v04	Sun Feb 10 2008
// 	verwende logstreams fuer besser organisierte Ein-/Ausgabe
// 	tests fuer Treffer in einer bestimmten Lage werden nun mit
// 	bitweisem "und" getestet statt mit Gleichheit
// 		Manuel Schiller <schiller@physi.uni-heidelberg.de>
// v05	Tue Mar 25 2008
// 	verbesserte Routinen zur Nachpulserkennung, einige kleinere
// 	kosmetische Aenderungen
// 		Manuel Schiller <schiller@physi.uni-heidelberg.de>
//
////////////////////////////////////////////////////////////////////////
#include "fp13Analysis.h"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#include "logstream.h"

using namespace logstreams;

// Konstruktor
fp13Analysis::fp13Analysis(const string& ifilename, const string& ofilename) :
	eventCounter(0), analyzedCounter(0),
	inputFile((ifilename == "-")?cin:
			(* new ifstream(ifilename.c_str()))),
	outputFile(* new TFile(ofilename.c_str(), "RECREATE")),
	inputFileName(ifilename),
	outputFileName(ofilename),
	eventFlags(FlagNone)
{
	// Fehler fuer Eingabedatei aufgetreten? (D.h. ist Datei offen?)
	if (inputFile.fail()) {
		delete &outputFile;
		error << "Fehler beim Oeffnen der Eingabedatei " <<
			ifilename << "." << endl;
		throw;
	}
	// Ausgabedatei offen?
	if (outputFile.IsZombie()) {
		// falls die Daten von cin kommen, darf der Speicher
		// nicht zurueckgegeben werden
		if (&inputFile != &cin)
			delete &inputFile;
		error << "Fehler beim Oeffnen der Ausgabedatei " <<
			ofilename << "." << endl;
		throw;
	}

	// Histogramme buchen
	bookHistograms();

	// Platz in den Arrays reservieren, die spaeter das Event enthalten,
	// damit es gleich gross genug ist
	detectorHitMask.reserve(32);
	detectorHitTimes.reserve(32);
}

// Destruktor
fp13Analysis::~fp13Analysis()
{
	// Anzahl der verarbeiteten Events ausgeben
	info << string(72, '*') << endl <<
		setw(8) << eventCounter << " Ereignisse gelesen, davon " <<
		setw(8) << analyzedCounter << " Ereignisse analysiert." <<
		endl << string(72, '*') << endl << endl;

	outputFile.Write();
	outputFile.Flush();

	// allokierte Objekte freigeben
	if (&inputFile != &cin)
		delete (ifstream*) &inputFile;
	delete &outputFile;
}

////////////////////////////////////////////////////////////////////////
// PUBLIC MEMEBER FUNCTIONS
////////////////////////////////////////////////////////////////////////
// Einlesen und Formatieren der Daten
int fp13Analysis::readEvent() 
{
	// falls das Ende der Datei erreicht wurde, wird -1 zurueckgegeben
	if (inputFile.eof())
		return -1;

	// Statusreport alle 10000 Ereignisse
	if ((eventCounter % 10000) == 0) {
		info << "Ereignis " << setw(8) << eventCounter <<
			", davon " << setw(8) << analyzedCounter <<
			" analysiert." << endl;
	}

	// Initialisiere alle ereignisbezogenen Klassenvariablen
	detectorHitMask.clear();
	detectorHitTimes.clear();

	// string zum Einlesen einer Zeile aus dem Eingabefile
	string buf;
	// istringstream zum Lesen der Zahlen aus der Zeile in buf
	istringstream bufStream;
	// Format der Daten in dem Textinputfile:
	// Beispiel eines Ereignisses (Zerfall nach unten)
	//
	// Zeitbinnumerierung     Binaeres Detektorhitmuster     Zeitpunkt der Hits [ns]
	//         1                        127                            50
	//         2                        128                          2370
	//        ###
	//
	// Die Zeichenfolge ### markiert das Ende eines Ereignisses
	//
	// Mit eingelesenes carriage return '\r' aus Dateien mit DOS-Zeilen-
	// endkonvention wird im Folgenden geflissentlich ignoriert
	// Solange kein Fehler auftritt:
	unsigned merged = 0;
	while (getline(inputFile, buf).good()) {
		// Verlasse die Schleife, wenn die Zeichenfolge fuer das Ende
		// eines Ereignisses gefunden wurde. Ein einzelnes "###" am
		// Anfang des Ereignisses wird ueberlesen, da die Datenfiles
		// mit "###" anfangen...
		if (buf.find("###", 0) != string::npos) {
			if (detectorHitMask.empty())
				continue;
			else
				break;
		}

		// Wir wollen aus der Zeile in buf Zahlen in Variablen
		// lesen. Dazu muss bufStream ueber den neuen Inhalt von
		// buf Bescheid wissen.
		bufStream.str(buf);

		// Variablen zum Zwischenspeichern der ausgelesenen Daten
		int hitMask, hitTime;
		unsigned nBin;
		// Extrahieren der einzelnen Informationen aus dem Buffer
		// und Zwischenspeicherung
		bufStream >> nBin >> hitMask >> hitTime;
		// Auf Fehler beim Einlesen der drei Zahlen pruefen
		if (bufStream.fail()) {
			warn << "Ungueltig formatierte Daten in Ereignis " <<
				eventCounter << " Zeitbin " <<
				detectorHitMask.size() <<
				", mache trotzdem weiter." << endl;
			continue;
		}
		// Zeiten und Hitmasken kommen in je einen Vektor
		detectorHitMask.push_back(hitMask);
		detectorHitTimes.push_back(hitTime);
		// check for increasing times
		if (detectorHitMask.size() > 1) {
			int delay = hitTime -
				detectorHitTimes[detectorHitMask.size() - 2];
			int deltadata = hitMask ^
				detectorHitMask[detectorHitMask.size() - 2];
			if (delay <= 0)
				warn << "In Ereignis " <<
					(eventCounter + 1) << ": Zeit steigt"
					" nicht streng monoton an: Bin " <<
					detectorHitMask.size() << " delay " <<
					delay << " deltaData " << deltadata <<
					"." << endl;

			if (delay <= 20){ 
				detectorHitMask[detectorHitMask.size() - 2] = detectorHitMask[detectorHitMask.size() - 2] ^ hitMask;
				detectorHitMask.pop_back();
				merged++;
			}
		}
		// Ueberpruefen, ob konsistent mit der Zaehlung im Datenfile
		if (nBin != (detectorHitMask.size()+merged)) {
			// Wenn nicht, ist irgendetwas faul...
			// EventCounter wird erst erhoeht, wenn das Ereignis
			// komplett gelesen wurde, daher das "+ 1" unten
			warn << "In Ereignis " << (eventCounter + 1) <<
				": Zeitbinzaehlung unstimmig (Bin " <<
				detectorHitMask.size() << " in Eingabedaten "
				"erscheint als " <<
				nBin << " beim Lesen). merged = " << merged << endl;
		}
	}
	// OK, Event eingelesen oder Fehler aufgetreten

	// auf Lesefehler pruefen (nicht Ende der Eingabedatei)
	if (inputFile.bad()) {
		error << "Fehler beim Lesen der Eingabedatei " <<
			inputFileName << "." << endl;
		throw;
	}
	// falls das Ende der Datei erreicht wurde, wird -1 zurueckgegeben
	if (inputFile.eof() && detectorHitMask.empty())
		return -1;
	// falls nicht Ende der Datei aber trotzdem leeres Event, Fehler-
	// meldung ausgeben...
	if (detectorHitMask.empty()) {
		warn << "leeres Event in der Eingabedatei " <<
			inputFileName << "." << endl;
		throw;
	}

	// Zaehle die erfolgreich gelesenen Ereignisse
	eventCounter++;

	// Event erfolgreich gelesen
	return 0;
}

// Analyse der Daten und Fuellen der Histogramme
void fp13Analysis::analyze() 
{
	// Mitzaehlen, wie viele Ereignisse analysiert wurden
	++analyzedCounter;
	// Eintreten in das Outputfile
	outputFile.cd();
	// Flags zuruecksetzen
	eventFlags = FlagNone;
	lastMuonLayer = -1;

	// Fuelle Histogramm mit der Anzahl der Zeitbins, die einen Hit
	// enthalten
	h6->Fill(detectorHitMask.size());

	// Zusammenfuegen von Hits, die in zwei benachbarten Zeitbins
	// liegen, aber nur ein Zeitdifferenz von 10 ns haben
	mergeHitsWithSmallTimeDifference();

	// Loop ueber alle Detektorlagen
	for (int iDetectorLayer = 0; iDetectorLayer < nLayers;
			iDetectorLayer++) {
		// Loop ueber alle Zeitbins
		for (unsigned iTimeBin = 0; iTimeBin < detectorHitMask.size();
				iTimeBin++) {
			// Histogramme als Funktion der Detektorlage

			// Anzahl der Hits in den Detekorlagen
			// Teste, ob die aktuelle Lage im betrachteten
			// Zeitfenster getroffen wurde
			if (detectorHitMask[iTimeBin] &
					(1 << iDetectorLayer))
				h1->Fill(iDetectorLayer);
		}
		// Zeitpunkt des Hits im ersten Zeitbin in jeder Detektorlage 
		//   -- falls es noch einen ersten Zeitbin gibt und 
		//   -- falls ein solcher vorhanden war
		if (detectorHitMask.empty())
			continue;
		if (detectorHitMask[0] & (1 << iDetectorLayer))
			h21[iDetectorLayer]->Fill(
					detectorHitTimes[0]);
	}

	// wie weit ist das Myon im Detektor gekommen?
	// wird in der Membervariable lastMuonLayer gespeichert
	lastMuonLayer = determineLastLayerHitByIncomingMuon();
	// falls kein durchfliegendes Myon im ersten Zeitbin gefunden
	// wurde, brauchen wir auch nicht nach Zerfaellen oder
	// Nachpulsen suchen!
	if (-1 == lastMuonLayer) return;
	// Fuelle Histogramm, falls eine Lage unter der Lage 0
	// (alles ab 2. von oben) getroffen wurde
	h8->Fill(lastMuonLayer);
	// Falls es nur Hits im ersten Zeitbin gibt, dann beende die
	// Methode hier, da es weder Nachpulse noch Zerfaelle gibt
	if (2 > detectorHitMask.size()) return;

	// Loope ueber alle Zeitbins, um zu schauen, ob Zerfaelle oder
	// Nachpulse gefunden werden
	// Das nullte Zeitbin ist das, in dem das Einfallende Myon
	// beobachtet wurde, daher muss man erst ab dem ersten Zeitbin
	// nach Zerfaellen und Nachpulsen schauen
	for (unsigned iTimeBin = 1; iTimeBin < detectorHitMask.size();
			++iTimeBin) {
		// Zeitverzoegerung schon mal ausrechnen...
		int delay = detectorHitTimes[iTimeBin] - detectorHitTimes[0];
		// falls delay klein, werfen wir das Zeitbin weg, weil wir
		// den Detektor fuer so kleine Zeiten nicht gut genug
		// verstehen
		if (delay < minDelay) continue;

		// die Methoden, die die Existenz von Nachpulsen oder
		// Zerfaellen nach oben/unten pruefen, geben -1 zurueck,
		// falls nichts gefunden wurde, andernfalls wird die Lage
		// des Nachpulses/Zerfalls angegeben
		int where = -1;
	
		where = findDecayUpward(iTimeBin);
		if (-1 != where) {
			// OK, Zerfall nach oben gefunden - Flags setzen
			eventFlags = static_cast<EventFlags>(
				eventFlags | FlagDecayUp);
			// Detektorlage des Zerfalls nach oben
			h3->Fill(where);
			// Histogramm der Verzoegerungszeit fuer die
			// getroffene Detektorlage
			h23[where]->Fill(delay);
		}

		// Zerfall nach unten: Bestimme Lage und Verzoegerungszeit
		// FIXME: Diese Methode sollen Sie selbst schreiben; weiter
		// unten steht eine Version, die nur sagt, dass sie nichts
		// gefunden hat
		where = findDecayDownward(iTimeBin);
		if (-1 != where) {
			// OK, Zerfall nach unten gefunden - Flags setzen
			eventFlags = static_cast<EventFlags>(
				eventFlags | FlagDecayDown);
			// Detektorlage des Zerfalls nach unten
			h4->Fill(where);
			// Histogramm der Verzoegerungszeit fuer die
			// getroffene Detektorlage
			h24[where]->Fill(delay);
		}

		// Suche Nachpulse mit durchgehenden Myonen
		where = -1;
		// Fuelle die Histogramme, solange wir Nachpulse finden
		while (-1 != (where = findAfterpulsesUsingThroughGoingMuons(
						iTimeBin, where))) {
			// Nachpuls gefunden - Flags setzen
			eventFlags = static_cast<EventFlags>(
				eventFlags | FlagAfterpulseSimple);
			// Detektorlage des Nachpulses
			h5->Fill(where);
			// Histogramm der Verzoegerungszeit fuer die
			// getroffene Detektorlage
			h25[where]->Fill(delay);
		}

		// Verbesserte Nachpuls-Analyse
		// FIXME: Diese Methode sollen Sie selbst schreiben; weiter
		// unten steht eine Version, die nur sagt, dass sie nichts
		// gefunden hat. Ueberlegen Sie sich, wie man die Nachpuls-
		// erkennung, die oben verwendet wird verbessern kann!
		where = -1;
		// Fuelle die Histogramme, solange wir Nachpulse finden
		while (-1 != (where = findAfterpulsesImproved(
						iTimeBin, where))) {
			// Nachpuls gefunden - Flags setzen
			eventFlags = static_cast<EventFlags>(
				eventFlags | FlagAfterpulseImproved);
			// Detektorlage des Nachpulses
			h2->Fill(where);
			// Histogramm der Verzoegerungszeit fuer die
			// getroffene Detektorlage
			h22[where]->Fill(delay);
		}
	} // Ende des Loops ueber die Zeitbins

	// Fuelle Anzahl der beruecksichtigten Zeitfenster in dem Ereignis
	// Fuelle nur, falls ein Zerfall nach oben oder unten gefunden wurde
	if (eventFlags & FlagDecay)
		h7->Fill(detectorHitMask.size());	
	// Glueckwunsch - Analyse des Events ist hier beendet! ;)
}

unsigned long fp13Analysis::getNoOfEvents()
{ return eventCounter; }

unsigned long fp13Analysis::getNoOfAnalyzedEvents()
{ return analyzedCounter; }

////////////////////////////////////////////////////////////////////////
// PRIVATE MEMEBER FUNCTIONS
////////////////////////////////////////////////////////////////////////

// Erstellen der benoetigten Histogramme
void fp13Analysis::bookHistograms()
{
	// Eintreten in das Output file
	// alle gebuchten Histogramme werden mit dem File assoziiert
	outputFile.cd();
	//
	// Buchen der Einzelhistogramme inklusive der Fehlerhistogramme
	h1 = new TH1D("h1",
		"Gesamtzahl der Hits in den Detektorlagen; Detektorlage; Anzahl der Hits",
		nLayers, -0.5, -0.5 + nLayers);
	h2 = new TH1D("h2",
		"Anzahl der Nachpulse in den Detektorlagen; Detektorlage; Anzahl der Nachpulse",
		nLayers, -0.5, -0.5 + nLayers);
	h3 = new TH1D("h3",
		"Lage der Zerfaelle nach oben; Lage; Anzahl der Hits",
		nLayers, -0.5, -0.5 + nLayers);
	h4 = new TH1D("h4",
		"Lage der Zerfaelle nach unten; Lage; Anzahl der Hits",
		nLayers, -0.5, -0.5 + nLayers);
	h5 = new TH1D("h5",
		"Detektor der Nachpulse; Detektor; Anzahl der Hits",
		nLayers, -0.5, -0.5 + nLayers);
	h6 = new TH1D("h6",
		"Anzahl aller Slices; Slices; Anzahl der Hits",
		40, 0., 20.);
	h7 = new TH1D("h7",
		"Anzahl der genommenen Slices; Slices; Anzahl der Hits",
		40, 0., 20.);    
	h8 = new TH1D("h8",
		"Letzte vom einlaufenden Myon kontinuierlich getroffene Detektorlage; Detektorlage; Anzahl der Hits",
		nLayers, -0.5, -0.5 + nLayers);
	// Buchen der Histogramme in den Vektoren
	for (int iDetectorLayer = 0; iDetectorLayer < nLayers;
			iDetectorLayer++ ) {
		h21.push_back(new TH1D(Form("z%i",iDetectorLayer),
					Form("Zeit des ersten Hits fuer Lage %i; "
						"Zeit [ns]; Anzahl der Hits",
						iDetectorLayer ),
					40, 0.,   200.));
		h22.push_back(new TH1D(Form("x%i", iDetectorLayer), 
					Form("Nachpulse fuer Detektor %i; "
						"Zeit [ns]; Anzahl der Hits",
						iDetectorLayer),
					125, 0., 50000.));
		h23.push_back(new TH1D(Form("a%i", iDetectorLayer), 
					Form("Zerfall nach oben in Lage %i; "
						"Zeit [ns]; Anzahl der Hits",
						iDetectorLayer),
					125, 0., 50000.));
		h24.push_back(new TH1D(Form("b%i", iDetectorLayer), 
					Form("Zerfall nach unten in Lage %i; "
						"Zeit [ns]; Anzahl der Hits",
						iDetectorLayer),
					125, 0., 50000.));
		h25.push_back(new TH1D(Form("w%i", iDetectorLayer), 
					Form("Nachpulse2 fuer Detektor %i; "
						"Zeit [ns]; Anzahl der Hits",
						iDetectorLayer),
					125, 0., 50000.));
	}

	// Fehler auf bins ausrechnen: Inhalt(bin i) +/- sqrt(Inhalt(bin i))
	h1->Sumw2();
	h2->Sumw2();
	h3->Sumw2();
	h4->Sumw2();
	h5->Sumw2();
	h6->Sumw2();
	h7->Sumw2();
	h8->Sumw2();
	for (int iLayer = 0; iLayer < nLayers; ++iLayer) {
		h21[iLayer]->Sumw2();
		h22[iLayer]->Sumw2();
		h23[iLayer]->Sumw2();
		h24[iLayer]->Sumw2();
		h25[iLayer]->Sumw2();
	}
}


// Funktion zum Zusammenfassen von Hits, die nur 10 ns auseinander liegen
void fp13Analysis::mergeHitsWithSmallTimeDifference()
{
	// falls das Event keine Daten enthaelt, sind wir fertig
	if (detectorHitMask.empty())
	    return;
	// Loope vom zweiten bis zum letzten Zeitbin
	// Iteratoren zeigen auf die Elemente eines "Datenbehaelters"
	// (engl: container), in unserem Fall ein vector<int>,
	vector<int>::iterator jt = detectorHitTimes.begin() + 1;
	for (vector<int>::iterator it = detectorHitMask.begin() + 1;
			detectorHitMask.end() != it; ) {
		// Teste, ob der Hit aus aktuellem Zeitbin nur eine
		// TDC-Zeiteinheit (10 ns) Unterschied zum Hit im naechsten
		// Zeitbin hat
		if ((*jt  - *(jt - 1)) > 10 || (*it & *(it - 1))) {
			// mehr als 10 ns, also nur zum naechsten Zeitbin
			// gehen
			++it;
			++jt;
		} else {
			// ja, Unterschied ist nur 10 ns, also werden die zwei
			// Zeitbins zusammengelegt (d.h. der Myon-Durchgang
			// faellt genau auf die Ecke eines TDC-Zeitbins)

			// Fuege die beiden Bitmasken der Detektorhits mit
			// einem bitweisen ODER (|) zusammen und speichere
			// sie im aktuellen bin
			*(it - 1) |= *it;
			// als Zeitstempel wird der vom frueheren Bin
			// verwendet, da der naeher an der Gesamtzeit
			// liegt, d.h. *(jt - 1) bleibt, wie es ist

			// Elemente loeschen - erase gibt Iterator auf Element
			// hinter dem geloeschten zurueck
			it = detectorHitMask.erase(it);
			jt = detectorHitTimes.erase(jt);
		}
	}
}

// Bestimmung der letzten Detektorlage, die vom einlaufenden Myon beim
// kontinuierlichen Durchlaufen getroffen wurde
int fp13Analysis::determineLastLayerHitByIncomingMuon()
{
    // Loope ueber alle ausser die obersten beiden Detektorlagen, d.h. 2
    // ... nLayers, starte mit unterster Lage, da der maximale
    // kontinuierliche Durchgang zu bestimmen ist
    for (int iLayer = nLayers - 1; iLayer >= 1; --iLayer) {
	// Teste, ob bis Detektorlage iLayer ein kontinuierlicher
	// Durchgang vorliegt, d.h., ob alle Detektorlagen bis
	// einschliesslich Lage iLayer in Zeitbin 0 getroffen wurden
	//
	// Sei n die zu testende Detektorlage. Dann haben die folgenden
	// Ausdruecke das gewuensche Bitmuster:
	//
	// Ausdruck	in C++-Syntax		Bitmuster
	// 					n (n-1) (n-2) ... 2 1 0
	//
	// 2^(n+1)	(1 << (n+1))		1   0     0   ... 0 0 0
		// 2^(n+1) - 1	((1 << (n+1)) - 1)	0   1     1   ... 1 1 1
		if (detectorHitMask[0] == ((1 << (iLayer + 1)) - 1)) {
			// Detektorlage iLayer ist die letzte kontinuerlich
			// getroffene Lage, und wir sind fertig
			return iLayer;
		}
	}
	// gib -1 zurueck, da nichts passendes gefunden wurde
	return -1;
}

// Finden eines Muonzerfalls nach oben
//
// Eingabewert:	timeBin - Zeitbin, das auf Zerfall nach oben zu untersuchen ist
// Rueckgabewert: Detektorlage, in der Zerfall nach oben gefunden wurde
// 		  -1, falls nichts gefunden wurde
int fp13Analysis::findDecayUpward(int timeBin)
{
	// die Membervariable lastMuonLayer haelt fest, wie weit das Myon im
	// Detektor gekommen ist
	// falls Teilchen bis zur letzten Lage gekommen ist, sind wir
	// fertig (warum?)
	if ((nLayers - 1) == lastMuonLayer)
		return -1;
	
	// Teste, ob ein Teilchen nach oben emittiert wird
	// (der Test funktioniert wie oben in der Methode
	// determineLastLayerHitByIncomingMuon())
	if ((1 << lastMuonLayer) & detectorHitMask[timeBin]) {
		// das Zerfallselektron/-positron wird in der gleichen Lage
		// registriert, in der das Myon zuletzt gesehen wurde, also
		// lastMuonLayer
		return lastMuonLayer;
	}
	
	return -1;
}

// Finden eines Muonzerfalls nach unten
//
// Eingabewert:	timeBin - Zeitbin, das auf Zerfall nach unten zu untersuchen ist
// Rueckgabewert: Detektorlage, in der Zerfall nach unten gefunden wurde
// 		  -1, falls nichts gefunden wurde
int fp13Analysis::findDecayDownward(int timeBin)
{
	if ((nLayers - 1) == lastMuonLayer)
		return -1;
	
	// Teste, ob ein Teilchen nach oben emittiert wird
	// (der Test funktioniert wie oben in der Methode
	// determineLastLayerHitByIncomingMuon())
	if ((1 << (lastMuonLayer+1)) & detectorHitMask[timeBin]) {
		// das Zerfallselektron/-positron wird in der gleichen Lage
		// registriert, in der das Myon zuletzt gesehen wurde, also
		// lastMuonLayer
		return lastMuonLayer;
	}
	// gib -1 zurueck, falls kein Zerfall nach unten stattfand
	return -1;
}

// Finden von Nachpulsen anhand durchgehender Myonen
//
// Eingabewert:	timeBin - Zeitbin, das auf Nachpulse zu untersuchen ist
//              startLayer - Lage, ueber der gesucht wird
// Rueckgabewert: Detektorlage, in der Nachpuls gefunden wurde
// 		  -1, falls nichts gefunden wurde
int fp13Analysis::findAfterpulsesUsingThroughGoingMuons(int timeBin,
		int startLayer)
{
	// die Membervariable lastMuonLayer haelt fest, wie weit das Myon im
	// Detektor gekommen ist (oder ist -1, falls kein Myon da ist)
	// falls das Muon nicht den gesamten Detektor durchflogen hat,
	// muessen wir hier nichts tun
	if ((nLayers - 1) != lastMuonLayer)
		return -1;
	// falls startLayer -1 ist, fangen wir neu an, also sinnvollen Wert
	// setzen: wir fangen mit der zweituntersten Lage an (warum?)
	if (-1 == startLayer)
		startLayer = nLayers - 1;

	// wir suchen im Zeitbin timeBin nach Nachpulsen
	// dabei arbeiten wir uns im Detektor nach oben, und fangen eine Lage
	// ueber der an, wo wir zuletzt etwas gefunden haben
	for (int iLayer = startLayer - 1; iLayer >= 0; iLayer--) {
		// teste, ob nur in der Lage iLayer ein Puls registriert wurde
		if ((1 << iLayer) & detectorHitMask[timeBin]) {
			// ja, gib die Lage zurueck
			return iLayer;
		}
	}

	// keine Nachpulse gefunden
	return -1;
}


// Finden von Nachpulsen (verbesserte Version)
//
// Eingabewert:	timeBin - Zeitbin, das auf Nachpulse zu untersuchen ist
//              startLayer - Lage, ueber der gesucht wird
// Rueckgabewert: Detektorlage, in der Nachpuls gefunden wurde
// 		  -1, falls nichts gefunden wurde
//
// FIXME: Ueberlegen Sie sich, was im Vergleich zur einfachen Nachpuls-
// erkennung oben verbessert werden kann, und implementieren Sie Ihre
// Verbesserung(en)
int fp13Analysis::findAfterpulsesImproved(int timeBin, int startLayer)
{
	
	
	if (-1 == startLayer)
		startLayer = nLayers - 1;

	
	
	for (int iLayer = startLayer - 1; iLayer >= 0; iLayer--) {
		
		
		if ((1 << iLayer) & detectorHitMask[timeBin]) {
			
			
			return iLayer;
		}
	}

	
	return -1;
}

// Dateiende

///////////////////////////////////////////////////////////////////////
// FP13 -- Lebensdauer von Myonen
//
// Executable zur Datenanalyse im Rahmen von FP13
//
// v00: Wed Apr 26 12:22:13 CEST 2006
// 	basierend auf Code von Sarah Dambach
//	aktuelle Verantwortliche:
//		Johannes Albrecht <albrecht@physi.uni-heidelberg.de>
//		Stefan Schenk <schenk@physi.uni-heidelberg.de>
//
// v01: Tue Sep 25 2007
// 	einige Cleanups fuer bessere Lesbarkeit und (hoffentlich)
// 	beispielhafteren Code
// 		Manuel Schiller <schiller@physi.uni-heidelberg.de>
//
// usage: fp13.exe [-n nEvents] [-i inputDataFile] [-o rootOutputFile]
//
// werden Eingabe- oder Ausgabefile auf der Kommandozeile nicht
// angegeben, so verwendet das Programm "fp13.txt" standardmaessig als
// Eingabefile und "fp13.root" als Ausgabefile
////////////////////////////////////////////////////////////////////////

// C++ header files
#include <iostream>
#include <string>
#include <sstream>
// C header files (fuer getopt)
#include <unistd.h>

// C++ header fuer logstream, die Programmeldungen netter machen
#include "logstream.h"
// C++ header file fuer das Analyseobjekt
#include "fp13Analysis.h"

using namespace std;
using namespace logstreams;

// Standardwerte der Eingabeparameter
static const unsigned long defMaxNoOfEvents = (unsigned long) -1;
static const unsigned long defFirstEvent = 0;
static const char *defInputDataFileName = "fp13.txt";
static const char *defRootOutputFileName = "fp13.root";

// kleine Prozedur zum Ausgeben eines Hilfetextes
void help(char *myname)
{
	cout << endl << "usage:\t" << myname << " [-n maxNoOfEvents] " <<
		"[-i inputDataFileName] " << "[-o rootOutputFileName] " <<
		"[-s skipNrEvents] [-q] [-v]" << endl << endl <<
		"\tIf not specified on the command line, input is read "
		"from " << defInputDataFileName << "," << endl <<
		"\tand output is written to " << defRootOutputFileName <<
		"." << endl << "\tUnless otherwise specified, at most " <<
		defMaxNoOfEvents << " events are processed." << endl <<
		"\tIf \"-\" is given as name of the input file, input " <<
		"data" << endl << "\tis read from stdin." << endl <<
		"\tThe options -q and -v make " << myname << " more quiet"
		" or more verbose and may" << endl <<
		"\tbe given more than once." << endl;
}

int main(int argc, char *argv[]) 
{
	// Setzte Standardwerte fuer die Anzahl der zu prozessierenden
	// Events und die Namen von Ein- und Ausgabedateien
	// Diese Standardwerte werden von Vorgaben durch Programmoptionen
	// weiter unten ueberschrieben.
	unsigned long maxNoOfEvents = defMaxNoOfEvents;
	// Anzahl zu ueberspringender Events
	unsigned long firstEvent = defFirstEvent;
	string inputDataFileName = defInputDataFileName;
	string rootOutputFileName = defRootOutputFileName;

	// Lese Programmoptionen aus
	int c;
	while ((c = getopt(argc, argv, "hqvn:i:o:s:")) != -1) {
		switch (c) {
			case 'n':// Anzahl der zu verarbeitenden Ereignisse
				// istringstream vorbereiten, aus dem die
				// Anzahl der zu verarbeitenden Events
				// gelesen wird
				{
				  istringstream stream(optarg);
				  stream >> maxNoOfEvents; // Lesen
				}
				break;
			case 's':// Anzahl der Events, die am Anfang ueber-
				// sprungen werden soll
				{
				  istringstream stream(optarg);
				  stream >> firstEvent; // Lesen
				}
				break;
			case 'i':// Name der Eingabedatei
				inputDataFileName = optarg;
				break;
			case 'o':// Name der Ausgabedatei
				rootOutputFileName = optarg;
				break;
			case '?':// Unbekannter Optionsbuchstabe
			case 'h':// Hilfe
				help(argv[0]);
				return -1;
			case 'q':// Ausgabe weniger ausfuerhlich
			case 'v':// Ausgabe ausfuehrlich
				logstream::setLogLevel(logstream::logLevel()
					+ (('q' == c) ? (1) : (-1)));
				break;
			default:// Optionsbuchstabe oben im getopt-Aufruf
				// als gueltig ausgewiesen, aber kein Code
				// da, um diesen Fall zu behandeln
				cerr << argv[0] <<
					": Unhandled option character \"-" <<
					c << "\"." << endl;
				return -1;
		}
	}

	// Benutzer informieren, was getan wird
	info << endl << string(72, '*') << endl <<
		"Eingabedatei:\t\t" << inputDataFileName << endl <<
		"Ausgabedatei:\t\t" << rootOutputFileName << endl <<
		"Bearbeite maximal\t" << maxNoOfEvents << " Ereignisse" << endl <<
		"Ueberspringe\t\t" << firstEvent << " Ereignisse" << endl <<
		string(72, '*') << endl << endl;

	// Erzeuge Instanz der Analyseklasse
	// Das Oeffnen der Dateien und Buchen der Histogramme uebernimmt
	// der Konstruktor, das Speichern der Histogramme und schliessen
	// der Dateien der Destruktor
	fp13Analysis *analysisObject = 
		new fp13Analysis(inputDataFileName, rootOutputFileName);

	// Ueberspringe Events, falls das gewuenscht wird
	while (analysisObject->getNoOfEvents() < firstEvent) {
		// Die readEvent Methode gibt einen Wert ungleich 0
		// zurueck, falls das Ende der Datei erreicht wurde.
		// In diesem Fall sind wir fertig.
		if (analysisObject->readEvent() != 0)
			return 0;
	}
		
	// Loope ueber alle verbleibenden Ereignisse
	while (analysisObject->readEvent() == 0) {
		// Analysiere das Ereignis
		analysisObject->analyze();
		// Abbrechen, falls gewuensche Anzahl Ereignisse analysiert
		if (analysisObject->getNoOfAnalyzedEvents() >=
				maxNoOfEvents)
			break;
	}

	// die Instanz der Analyseklasse loeschen und den Speicher freigeben
	// (der delete-Operator ruft den Destruktor der Klasse)
	delete analysisObject;

	// wir sind fehlerfrei durchgelaufen und geben daher 0 zurueck
	return 0;
}

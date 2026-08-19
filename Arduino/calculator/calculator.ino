// Mikrocontroller-Rechner: empfängt "Zahl + Operator + Zahl" (z.B. "34*72"),
// führt die Rechnung aus und sendet das Ergebnis zurück an den PC.

void setup() {
  Serial.begin(9600);
  while (!Serial) { ; }
}

// Prüfung,ob bei der Schnittstelle etwas ankommt oder nicht
void loop() {

  if (Serial.available() > 0) {                
    // Erkennung mit dem Ende der Nachricht/Befehl vom PC
    String eingabe = Serial.readStringUntil('\n');
    // Entfernung von Überresten, Leerzeichen…
    eingabe.trim(); 

    if (eingabe.length() > 0) {
      // Leere Anfragen werden nicht bearbeitet
      verarbeiteAusdruck(eingabe);
    }
  }
}

// Zerlegung des Strings in 3 Teile; 1.Zahl, Rechenoperator, 2.Zahl + Ausführung der Rechnung
void verarbeiteAusdruck(String ausdruck) {
  // Als Erstes Suchen der Rechenoperator-Position (erstes Auftauchen von + - * /)
  int operatorIndex = -1;
  char operatorZeichen = ' ';

  for (int i = 0; i < ausdruck.length(); i++) {
    char c = ausdruck.charAt(i);
    // Auslesung des Rechenoperators; Minus-Operator mit Extra-Bedingung
    if ((c == '+' || c == '*' || c == '/' || (c == '-' && i > 0))) {
      operatorIndex = i;
      operatorZeichen = c;
      break;
    }
  }
  // Prüfung, ob ein Rechenoperator vorhanden ist
  if (operatorIndex == -1) {
    Serial.println("ERR:Syntax");
    return;
  }

  // Teilung des Strings in zwei Zahlteile
  String teilA = ausdruck.substring(0, operatorIndex); 
  String teilB = ausdruck.substring(operatorIndex + 1);

  teilA.trim();
  teilB.trim();

  // Prüfen, ob beide Teile gültige Zahlen sind
  if (!istGueltigeZahl(teilA) || !istGueltigeZahl(teilB)) {
    Serial.println("ERR:Syntax");
    return;
  }

  // Änderung des Strings/Texts in eine echte Zahl
  double zahlA = teilA.toDouble();
  double zahlB = teilB.toDouble();
  double ergebnis = 0;

  // Ausführung der Rechnung
  switch (operatorZeichen) {
    case '+':
      ergebnis = zahlA + zahlB;
      break;
    case '-':
      ergebnis = zahlA - zahlB;
      break;
    case '*':
      ergebnis = zahlA * zahlB;
      break;
    case '/':
      if (zahlB == 0) {
        Serial.println("ERR:DivZero");
        return;
      }
      ergebnis = zahlA / zahlB;
      break;
    default:
      Serial.println("ERR:Syntax");
      return;
  }

  // Overflow-Pruefung, damit der Mikrocontroller  das Ergebnis sauber darstellen kann
  const double GRENZE = 2000000000.0; // knapp unter Wertebereich-Obergrenze von long (2.147 Mrd.)
  if (ergebnis > GRENZE || ergebnis < -GRENZE) {
    Serial.println("ERR:OVERFLOW");
    return;
  }

  // Sendung Ergebnis
  Serial.print("OK:");
  if (ergebnis == (long)ergebnis) {
    Serial.println((long)ergebnis);
  } else {
    Serial.println(ergebnis, 4); // 4 Nachkommastellen
  }
}

// Prüft, ob ein String eine gültige Zahl darstellt 
bool istGueltigeZahl(String s) {
  if (s.length() == 0) return false;

  int start = 0;
  if (s.charAt(0) == '-') {
    start = 1;
    if (s.length() == 1) return false; // nur "-" ist keine Zahl
  }

  for (int i = start; i < s.length(); i++) {
    if (!isDigit(s.charAt(i))) return false;
  }

  return true;
}

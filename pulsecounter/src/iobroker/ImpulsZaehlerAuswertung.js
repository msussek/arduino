// von https://github.com/Garfonso/arduino/blob/master/pulsecounter/src/iobroker/ImpulsZaehlerAuswertung.js
// Gas-Zähler, von Impulsen zu Messwert.

const idBrennerAktiv = 'javascript.' + instance + '.Gas.BrennerAktiv';
createState(idBrennerAktiv, false, {"type": "boolean", read: true, write: false, role: "indicator.working"});
const idBrennerlaufzeit = 'javascript.' + instance + '.Gas.Brennerlaufzeit';
createState(idBrennerlaufzeit, 0, {"type": "number", read: true, write: false, role: "value.interval", unit: "sec"});
const idBrennerstarts = 'javascript.' + instance + '.Gas.Brennerstarts';
createState(idBrennerstarts, 0, {"type": "number", read: true, write: false, role: "value"});
const idBrennwert = 'javascript.' + instance + '.Gas.Brennwert';
createState(idBrennwert, 10.265, {"type": "number", read: true, write: false, role: "value", unit: "kWh/m³"});
const idZustandszahl = 'javascript.' + instance + '.Gas.Zustandszahl';
createState(idZustandszahl, 0.9655, {"type": "number", read: true, write: false, role: "value"});
const idLeistung = 'javascript.' + instance + '.Gas.Leistung';
createState(idLeistung, 0.9655, {"type": "number", read: true, write: false, role: "value"});
const idVerbrauchEnergie = 'javascript.' + instance + '.Gas.Verbrauch_Energie';
createState(idVerbrauchEnergie, 0, {"type": "number", read: true, write: false, role: "value.power.consumption", unit: "kWh"});
const idVerbrauchVolumen = 'javascript.' + instance + '.Gas.Verbrauch_Volumen';
createState(idVerbrauchVolumen, 0, {"type": "number", read: true, write: false, role: "value.gas.consumption", unit: "m3"});

on({id: 'mqtt.0.SmartHome.Sensor.Haustechnikraum.Impulszaehler.Zaehler_0.Impulse', change: "any"}, function (obj) {
  const value = obj.state.val;
  const oldValue = obj.oldState.val;
  // Zeitdifferenz seit letzter Änderung in ms
  const deltaZeit = (new Date().getTime()) - getState(idVerbrauchVolumen).lc;
  // Anzahl Impulse seit letzter Änderung
  const deltaWert = value - oldValue;
  if (deltaWert > 0) {
    // Wenn Zähler läuft
    if (!getState(idBrennerAktiv).val) {
      setState(idBrennerAktiv, true, true);
      setState(idBrennerstarts, (getState(idBrennerstarts).val + 1), true);
    }
    // Verbrauch in m³
    const verbrauchInKubikmeter = Math.round((getState(idVerbrauchVolumen).val + deltaWert * 0.01)*100)/100;
    setState(idVerbrauchVolumen, verbrauchInKubikmeter, true);
    // Verbrauch in kwh
    const verbrauchInKwh = Math.round((verbrauchInKubikmeter * getState(idBrennwert).val * getState(idZustandszahl).val)*100)/100;
    setState(idVerbrauchEnergie, verbrauchInKwh, true);
    // gasVolumenStrom in m³/s
    const gasVolumenStrom = (deltaWert * 0.01) / (deltaZeit / 1000);
    // leistung in W
    const leistung = Math.round(gasVolumenStrom * getState(idBrennwert).val * getState(idZustandszahl).val * 3600000);
    setState(idLeistung, leistung, true);
  } else {
    // Wenn Zähler nicht läuft
    if (getState(idBrennerAktiv).val) {
      const deltaZeit = (new Date().getTime()) - getState(idBrennerAktiv).lc;
      setState(idBrennerAktiv, false, true);
      setState(idLeistung, 0, true);
      setState(idBrennerlaufzeit, (Math.round(getState(idBrennerlaufzeit).val + deltaZeit / 1000)), true);
    }
  }
});

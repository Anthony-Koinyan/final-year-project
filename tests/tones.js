import * as gpio from "gpio";
import { setTimeout, clearTimeout, setInterval, clearInterval } from "timers";

const BUZZER_PIN = 4;
const TEMPO_MULTIPLIER = 1.5;

const NOTES = {
  R: 0,
  C4: 262,
  G4: 392,
  D4: 294,
  A4: 440,
  E4: 330,
  B4: 494,
  F4: 349,
  C5: 523,
  F5: 698,
};

const twinkleTwinkle = [
  ["C4", 250],
  ["C4", 250],
  ["G4", 250],
  ["G4", 250],
  ["A4", 250],
  ["A4", 250],
  ["G4", 500],
  ["R", 250],
  ["F4", 250],
  ["F4", 250],
  ["E4", 250],
  ["E4", 250],
  ["D4", 250],
  ["D4", 250],
  ["C4", 500],
  ["R", 500],
];

const maryHadALittleLamb = [
  ["E4", 250],
  ["D4", 250],
  ["C4", 250],
  ["D4", 250],
  ["E4", 250],
  ["E4", 250],
  ["E4", 500],
  ["D4", 250],
  ["D4", 250],
  ["D4", 500],
  ["E4", 250],
  ["G4", 250],
  ["G4", 500],
  ["R", 500],
];

const odeToJoy = [
  ["E4", 250],
  ["E4", 250],
  ["F4", 250],
  ["G4", 250],
  ["G4", 250],
  ["F4", 250],
  ["E4", 250],
  ["D4", 250],
  ["C4", 250],
  ["C4", 250],
  ["D4", 250],
  ["E4", 250],
  ["E4", 375],
  ["D4", 125],
  ["D4", 500],
  ["R", 500],
];

const playlist = [
  { name: "Twinkle Twinkle Little Star", melody: twinkleTwinkle },
  { name: "Mary Had a Little Lamb", melody: maryHadALittleLamb },
  { name: "Ode to Joy", melody: odeToJoy },
];

function playNote(pin, freq, duration, callback) {
  if (freq <= 0) {
    setTimeout(callback, duration);
    return;
  }
  const halfPeriodMs = 1000 / freq / 2;
  let state = false;
  const toggle = () => {
    state = !state;
    pin.write(state);
  };
  const interval = setInterval(toggle, halfPeriodMs);
  setTimeout(() => {
    clearInterval(interval);
    pin.write(false);
    callback();
  }, duration);
}

function playContinuously(pinNum) {
  console.log(`--- Starting Continuous Melody Player on GPIO ${pinNum} ---`);
  const buzzer = gpio.setup(pinNum, { mode: "output" });
  let playlistIndex = 0;

  const playNextMelody = () => {
    if (playlistIndex >= playlist.length) {
      playlistIndex = 0;
    }
    const currentSong = playlist[playlistIndex];
    console.log(`Now playing: ${currentSong.name}`);
    let noteIndex = 0;

    const playNextNote = () => {
      if (noteIndex >= currentSong.melody.length) {
        console.log("Melody finished. Pausing before next song...");
        setTimeout(() => {
          playlistIndex++;
          playNextMelody();
        }, 1500);
        return;
      }
      const [noteName, baseDuration] = currentSong.melody[noteIndex];
      const duration = baseDuration * TEMPO_MULTIPLIER;
      const freq = NOTES[noteName];
      noteIndex++;
      playNote(buzzer, freq, duration, playNextNote);
    };
    playNextNote();
  };
  playNextMelody();
}

playContinuously(BUZZER_PIN);

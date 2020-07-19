/* ----------------------------------------------------------------------------

   ProtoFaust
   ==========
   DSP prototyping in Faust for VCV Rack

   Copyright (c) 2019-2020 Martin Zuther (http://www.mzuther.de/) and
   contributors

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Thank you for using free software!

---------------------------------------------------------------------------- */


// Converts 1 V/oct to frequency in Hertz.
//
// The conversion formula is: 440 * 2 ^ (volts - 0.75)
// The factor 0.75 shifts 0 V to C-4 (261.6256 Hz)
cv_pitch2freq(cv_pitch) = 440 * 2 ^ (cv_pitch - 0.75);


// Converts frequency in Hertz to 1 V/oct.
//
// The conversion formula is: log2(hertz / 440) + 0.75
// The factor 0.75 shifts 0 V to C-4 (261.6256 Hz)
freq2cv_pitch(freq) = ma.log2(freq / 440) + 0.75;


// Converts 200 mV/oct to frequency in Hertz.
i_cv_pitch2freq(i_cv_pitch) = i_cv_pitch : internal2cv_pitch : cv_pitch2freq;


// Converts frequency in Hertz to 200 mV/oct.
freq2i_cv_pitch(freq) = freq : freq2cv_pitch : cv_pitch2internal;


// Converts Eurorack's 1 V/oct to internal 200 mv/oct.
cv_pitch2internal(cv_pitch) = cv_pitch / 5;


// Converts internal 200 mv/oct to Eurorack's 1 V/oct.
internal2cv_pitch(i_cv_pitch) = i_cv_pitch * 5;


// Converts Eurorack's CV (range of 10V) to internal CV (range of 1V)
cv2internal(cv) = cv / 10;


// Converts internal CV (range of 1V) to Eurorack's CV (range of 10V)
internal2cv(i_cv) = i_cv * 10;

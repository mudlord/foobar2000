0.43:
* Tightened checks on metadb_handle_ptr being used for pitch shift metadata.

0.42:
* Rewrote sample buffer allocation logic for pitch/tempo/playback rate rate.
* Flush audio data for pitch/tempo/playback rate properly this time.
* Updated SoundTouch.

0.41
* Tried to mitigate crashes in DirectX DMO based DSPs.
* Added tag parsing for pitch DSP when enabled.
  - "pitch_amt" metadata can be used to specify pitch.
  - Specify pitch offset in semitones eg. "5.0" or "-5.0"

0.40
* Reorganized all dialogs, fixed sizing. (beta 1)
* Fixed issues with changing filter in IIR filter UI element. (beta 1)
* Added editable "Q" for IIR filter. (beta 1)
* Moved pitch/tempo/rate UI elements back into 1 dialog. (beta 1)
* Fixed IIR filter selection in UI element when disabled (beta 2)
* Fixed tempo selection in UI element when disabled (beta 2)
* Fixed typo for echo UI element selection (beta 3)
* IIR filter now defaults to proper variables (beta 3)
* Fixed playback rate setting (beta 4)
* Added DirectX based audio compressor (beta 4)
* Added DirectX based chorus effect (beta 4)
* Added DirectX based distortion effect (beta 5)
* Added DirectX based gargle effect (beta 5)
* Added DirectX based reverb effect (beta 5)
* Updated SoundTouch (beta 5)
* Fixed distortion effect activation (beta 6)
* Fixed seeking when using pitch/tempo/rate effects on fb2k 1.5 >

0.35
* Added reset functionality to pitch/rate/tempo DSPs
- Can now easily reset sliders to 0
- Setting the pitch/rate/tempo DSPs to 0 now just passes through
samples.
* Ported over the GUI changes from the pitch/tempo/rate 
UI elements to the normal DSP config manager equivalents.
- Now you can also type in the exact values (to 2 decimal points)
in the dialogs.

0.34
* Minor change to subclassed control for pitch/rate/tempo editing.

0.33
* Fix playbackrate controls.

0.32
* Pitch/tempo/rate UI elements now have the edit box focused first.
* Added finer granularity to playback shift DSP.

0.31.x
* Rewrote pitch/tempo/playback rate UI elements.
  Now they have directly editable pitch/tempo/rate

0.30.3
* Fixed crashes on extreme tempo changes
* Fixed accuracy of tempo slider for Rubberband tempo algorithm

0.30.2
* Added finer granularity to tempo shifting operations.

0.30.1:
* Attempt fix of "illegal instruction" crash on ancient CPUs.
- Should now just require SSE1 instead of SSE2 (MSVC2017 default)
* Readd tempo algorithm switcher in the tempo DSP.

0.30:
* Moved to most recent SDK (requires foobar2000 1.4)
  - Removed redundant code, now uses DSP preset appending code in SDK.
  - Ported GUIs to foobar2000 Default UI elements.
* Removed dynamics compressor DSP, moved to seperate DSP component.
* Wrapped all classes in anonymous namespaces.
* Changed to a dynamic MSVC runtime.
* Rewrote pitch/tempo/rate DSPs.
* Updated SoundTouch to latest Subversion version.
* Updated RubberBand to latest Mercurial version.
* Reverb coefficients are now scaled according to samplerate.
LTA LABS — VOLUME 1 — 9 WINDOWS VST3 EFFECTS — SOURCE PROJECT
===========================================================

This is a JUCE 8.0.6 / CMake project containing NINE DIFFERENT VST3 plugins.
It is SOURCE CODE, not a precompiled plugin download.

PLUGINS
  1  LTA Skinwalker       Vocal spectral colour / robotic & monster textures
  2  LTA Dream Journal    Granular memory fragments and hazy echoes
  3  LTA Moshpit          Compressor, transient emphasis, distortion
  4  LTA Eclipse          Blended low/high-pass shadow filtering
  5  LTA Redacted         Rhythmic blackouts, repeats and filtering
  6  LTA Second Sun       Parallel upper harmonics, widening
  7  LTA Nightmare Fuel   Dark unstable delayed textures
  8  LTA X-Ray            Sidechain-aware dynamic low-frequency reduction
  9  LTA False Memory     Worn, blurred and drifting memory effect

IMPORTANT FIRST-VERSION LIMITATIONS
- These are working DSP implementations intended as V1 prototypes, NOT
  proven commercial-grade processors. A GitHub compilation and in-FL test
  are still required. No downloaded Windows binaries are included here.
- Skinwalker offers formant-coloured robot/whisper/monster effects, not a
  professional independent pitch/formant shifter.
- Dream Journal records continuously into a rolling buffer and replays
  randomized grains, rather than writing separate editable memory clips.
- Redacted is a free-running rhythmic gate; this version is not tempo-synced.
- X-Ray accepts optional sidechain input and applies responsive low-band
  reduction. It does not claim to measure psychoacoustic masking using FFT.
- Six fully automatable parameters per plugin, including dry/wet and output.
- All nine are independently identifiable in FL Studio and can be used on
  mixer tracks. Keep master output low when experimenting.

BUILD IN GITHUB (NO VISUAL STUDIO INSTALL REQUIRED ON YOUR PC)
1. Create a NEW empty PUBLIC repository at https://github.com/new
   Suggested name: lta-labs-vst-bundle
2. Unzip this source package locally.
3. In the new repo choose Add file > Upload files. Drag the project files
   and folder 'Shared', with all contents preserved, and Commit changes.
4. IMPORTANT: GitHub's browser uploader may not upload the hidden .github
   directory. Verify the following file exists on GitHub at EXACTLY:
      .github/workflows/build-windows.yml
   If not, Code > Add file > Create new file, use that pathname in the name
   box, then paste all text from GITHUB_WORKFLOW_COPY_PASTE.yml and commit.
5. Open Actions > Build LTA LABS Vol 1 - Nine VST3 Plugins.
6. Select Run workflow > main > Run workflow. Do not choose Re-run jobs
   after making changes; a NEW run is needed to use the new commit.
7. Open the successful GREEN-tick run and scroll down to Artifacts.
   Download 'LTA-LABS-VOL1-Windows-VST3'.
8. Windows may give you ZIP INSIDE ZIP. Extract both until there are 9
   folders ending in .vst3 (each .vst3 is a whole folder, not an EXE).
9. CLOSE FL Studio. Copy all NINE .vst3 FOLDERS into:
      C:\Program Files\Common Files\VST3
   You may be asked for administrator permission.
10. Open FL Studio > Options > Manage plugins > Find installed plugins.
    Search 'LTA Skinwalker' etc. Put them into mixer FX slots.

If Actions page only displays a 'Get started' template page, the workflow
file is missing. Step 4 fixes this without reuploading everything.
If a red cross appears, open the failing job and copy the FIRST compiler
error, not just the final red summary line.

NOTES
- Distinct VST3 IDs: Ltal/SkW1 DrJ1 MsP1 EcL1 RdC1 ScS1 NmF1 XrA1 FlM1.
- Shared source is compiled separately for each target with plugin index.
- Source changes need a fresh workflow dispatch to generate a new artifact.
- The ZIP is meant to be uploaded/extracted, not executed as an installer.

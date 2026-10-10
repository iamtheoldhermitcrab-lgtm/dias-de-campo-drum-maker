# Días de Campo Drum Maker

Plugin VST3 (Windows y Mac) y AU (Mac) para Ableton Live que genera **patrones de batería** de minimal, rominimal, minimal house, deep house, house clásico, UK garage, UK bassline y acid house, y los toca con su propia **caja de ritmos sintetizada** (nada de samples). Es el cuarto de la familia, después del **Bass Maker**, el **Stab Maker** y el **Chord Maker**. Comparte con ellos la forma de trabajar, esta vez en **burdeos** y con la escena de la **carpa de baile de noche**.

Hecho por **Old Hermit Crab**, **Atmoodsphere** y **Hessa**.

## 1. Conseguir el plugin compilado

Igual que con los otros:

1. En GitHub crea un repositorio nuevo, por ejemplo `dias-de-campo-drum-maker`, en **Private**. El nombre **no puede acabar en punto**.
2. Pulsa **uploading an existing file** y arrastra desde el Explorador de Windows **todo** lo que hay dentro de la carpeta descomprimida: `Source`, `Resources`, `test`, `.github`, `CMakeLists.txt`, `LEEME.md` y `PROYECTO.md`.
   - Para ver la carpeta `.github`, activa **Vista → Elementos ocultos**.
   - Pulsa **Commit changes**.
   - Si `.github/workflows/build.yml` no se sube, ve a **Actions → set up a workflow yourself**, pega el contenido de ese archivo y pulsa **Commit changes**.
3. En **Actions**, cuando salga el check verde, descarga **DiasDeCampoDrumMaker-VST3-Windows** (o **DiasDeCampoDrumMaker-Mac**).

## 2. Instalarlo

- **Windows**:
  1. Copia la carpeta `Dias de Campo Drum Maker.vst3` en `C:\Program Files\Common Files\VST3\`.
  2. Abre Ableton → **Options → Preferences → Plug-Ins → Rescan**.
  3. El plugin aparece en **Plug-Ins → Dias de Campo**.
- **Mac**:
  1. Copia el `.component` en `~/Library/Audio/Plug-Ins/Components` y el `.vst3` en `~/Library/Audio/Plug-Ins/VST3`.
  2. En Terminal, ejecuta:
     ```
     xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/"Dias de Campo Drum Maker.component" ~/Library/Audio/Plug-Ins/VST3/"Dias de Campo Drum Maker.vst3"
     ```

## 3. Cómo se usa

### Generar

**GENERAR** crea un patrón de 10 instrumentos según el **Estilo** y elige un kit, un groove, un swing y una compresión que van con él. **Mantener** bloquea el sonido, el patrón o los FX.

### La rejilla

Hay una fila por instrumento: BOMBO, CAJA, PALMAS, RIM, CH CERR, CH ABIER, RIDE, TOM, SHAKER y PERC.

- **Clic en el nombre**: elige la fila. Los diales Nivel, Paneo, Color, Tono y Decay y el carril VELOCITY pasan a ese instrumento.
- **Clic derecho en el nombre**: abre un menú para:
  - cambiar la **longitud** de esa fila (polimetría: una percusión de 6 o 10 pasos que va rotando contra el bombo);
  - rehacer solo esa fila;
  - borrarla;
  - silenciarla o dejarla sola.
- Los golpes apagados son los que solo suenan con más **Densidad**. Ese dial quita y pone golpes en directo.
- El dado bajo cada compás rehace solo ese trozo.

### Herramientas

- **Poner/quitar**: clic pone o quita un golpe; arrastra para pintar o borrar varios.
- **Acento**: clic acentúa el golpe; clic derecho lo convierte en golpe fantasma (muy flojo).
- **Redoble**: 2, 3 o 4 golpes dentro del mismo paso.
- **Probab.**: el golpe suena el 100, 75, 50 o 25 % de las veces, así el patrón cambia solo cada vuelta.
- **Empuje**: retrasa el golpe (clic) o lo adelanta (clic derecho). Es el swing de cada golpe por separado.

### Pads

Hay un pad por instrumento, y se iluminan con cada golpe.

- **Clic**: escucharlo y elegir su fila.
- **M**: silenciarlo.
- **S**: dejarlo solo.

También se tocan con un teclado MIDI, con las notas GM (C1 = bombo, D1 = caja...).

### Selectores

- **Tono**: afina el bombo y el tom a la nota del tema, para que peguen con el bajo.
- **Fills**: un redoble al final de cada 4 u 8 compases. El dial **Fills** marca cuánto se llena.
- **Groove**:
  - **Recto**: a tiempo.
  - **Shuffle**: semicorcheas en ternario.
  - **Suelto**: algunos golpes caen un poco tarde, como tocado a mano.
- **Kit**: los sonidos. Hay seis:
  - **Nueve 909**
  - **Ocho-Ocho 808**
  - **Siete**
  - **Minimal** (seco, con clic)
  - **Garage**
  - **Lo-fi** (sucio, con menos bits)

### Diales

- **Densidad** y **Variación**.
- **Nivel**, **Paneo**, **Color**, **Tono** y **Decay** del instrumento elegido.
- **Compresión**: en paralelo, para más pegada.
- **Fills**, **Reverb** (el bombo queda seco), **Volumen**, **Swing** y **Humano**.

### Charles

El charles cerrado corta al abierto, como en una caja de ritmos de verdad.

### Igual que en los otros

- Banco A-H y Cadena.
- Carriles VELOCITY y MOOD, con el panel de MODULACIÓN. El Mood puede mover Densidad, Tono, Decay, Color, Compresión, Reverb o Volumen.
- Mis presets, guardados en `Documentos/Dias de Campo/Drum Maker Presets`.
- Arrastrar a Ableton en MIDI (notas GM, canal 10) y en WAV.
- Salida MIDI en tiempo real, para usar los patrones con un Drum Rack.

## 4. La carpa

- Es de noche: una carpa de circo de rayas con el DJ dentro, altavoces que laten, láseres, focos y guirnaldas de bombillas.
- La gente **salta con cada bombo**.
- Cada GENERAR sale alguien más de las tiendas a bailar, y la noche avanza hacia el amanecer.
- Cada 3 minutos pasan la avioneta con el cartel, el platillo volante (que se lleva a alguien, o al DJ) y el raver con jetpack.

## Licencias

- Construido con JUCE (https://juce.com).
- Las fuentes Press Start 2P y Pixelify Sans tienen licencia SIL Open Font License (ver `Resources/OFL-*.txt`).

# Días de Campo Bass Maker — ficha del proyecto

Hecho por **Old Hermit Crab**, **Atmoodsphere** y **Hessa**.
Esta ficha resume todo lo necesario para retomar el trabajo en cualquier conversación nueva con Claude:
pásale este archivo (o el repositorio) y dile "seguimos con el Bass Maker".

Última actualización: 9 de octubre de 2026.

---

## 1. Qué es

Plugin VST3 (Windows y Mac) y AU (Mac) para Ableton Live 12 que **genera líneas de bajo** de minimal house,
rominimal, deep house y UK house, y las toca con **su propio sinte** (58 sonidos). Estética de radio de camping
en **8 bits**, con una escena animada de un festival en el bosque.

- Repositorio: `iamtheoldhermitcrab-lgtm/dias-de-campo-bass-maker` (privado).
- Se compila solo con **GitHub Actions** (`.github/workflows/build.yml`): Windows (VST3) y Mac (VST3 + AU universal).
- Probado en Ableton 12 en varios PCs con Windows: funciona bien. **Falta probar en Mac.**

## 2. Cómo trabajamos (preferencias del autor)

- Todo en **español**, explicado de forma sencilla y práctica.
- Antes de cambiar algo visual, **enseñar primero una imagen de prueba** y esperar el visto bueno.
- Cambios **mínimos y concretos**: no cambiar lo que no se ha pedido.
- Entregar los archivos **sueltos**, diciendo a qué carpeta va cada uno (Source o raíz).
  Recordar: borrar copias viejas de Descargas para evitar nombres con " (1)" y respetar mayúsculas.
- Comprobar siempre antes de entregar: compila con gcc y clang, sin errores de memoria (ASan/UBSan),
  sin clics nuevos (prueba CLICKS), sin cambios de volumen de los sonidos.
- Nada de cadenas con acentos sin `u8(...)` / `String::fromUTF8` ni secuencias `\xNN` seguidas de letras hexadecimales
  (rompe en Windows/MSVC).

## 3. Estructura de archivos

| Archivo | Qué contiene |
|---|---|
| `Source/Pattern.h` | Patrones, pasos (nota, duración, acento, slide, velocity, densidad), presets (58 sonidos), parches analógicos y FM, estilos y **generador** de líneas (frases, motivos, progresiones, densidad por capas, regenerar por partes). |
| `Source/Synth.h` | **Motor de sonido**: voz mono (motores básico, FM de 2 y 4 operadores, analógico con filtros de retardo cero por familia, cuerda pulsada), envolventes tipo condensador, chorus tipo Juno (BBD), filtro de media banda para el oversampling. |
| `Source/PluginProcessor.h/.cpp` | Parámetros, reproducción sincronizada con Ableton, oversampling 2x, Mood/LFO, Pump, humanizar, cadena de patrones, sub mono, salida MIDI, presets de usuario, exportar MIDI y WAV, guardar en el proyecto. |
| `Source/PluginEditor.h/.cpp` | Toda la interfaz: radio, piano roll y carril en 8 bits reales, pantallas LED, diales, panel de modulación, presets, ventana redimensionable. |
| `Source/Look.h` | Estilo visual (colores, fuentes, pantallas LED, botones) y la **escena** del campamento (gente, DJ, día/noche, avioneta, platillo, jetpack). |
| `Source/LogoData.h` | Logos incrustados (DIAS DE CAMPO; Atmoodsphere sin uso ahora). |
| `Resources/` | Fuentes pixel (Press Start 2P, Pixelify Sans, licencia OFL). |
| `test/Test.cpp` | Programa de pruebas (`DdcTest`), se activa con `-DDDC_BUILD_TEST=ON`. Modos por variables de entorno: `NEW`, `CLICKS`, `LAYER`, `FEAT4`, `FILTERTEST`, `OSTEST` (+ `OSPRESET`, `OSOUT`, `OSLINE`, `OSKEEP`, `OSRESO`, `OSCUT`, `OSENV`, `SUBMONO`, `OSDEF`, `HUM`). |
| `CMakeLists.txt` | JUCE 8.0.4, C++20, formatos VST3 / AU / Standalone. `NEEDS_MIDI_OUTPUT TRUE`. |
| `LEEME.md` | Manual de usuario e instalación. |

## 4. Funciones hechas

**Generador**: estilos (Minimal, Minimal rolling, Minimal house, Deep house, UK garage, UK bassline, Acid house),
tono y escala, octava (-2..+2), longitud 1/2/4 compases, Variación, **Densidad en directo** (quita y pone notas sin
cambiar la línea), **Variar**, **regenerar por partes** (dado bajo cada compás), deshacer/rehacer, banco **A-H**,
**cadena de patrones** (A-B, A-D, A-H), transponer con el teclado MIDI.

**Edición**: piano roll de 4 octavas con rueda para desplazarse, herramientas Poner/Quitar, Acento, Slide, Alargar,
Octava; carril **VELOCITY** y carril **MOOD** (segundo secuenciador) con **LFO** sincronizado a cualquier destino.

**Sonido**: 58 bajos por familias (FM 4 operadores tipo DX7/TX81Z, Mono tipo SH-101, Coro tipo Juno-106, Ocho tipo
Jupiter-8, Seis tipo Polysix, Tres tipo JX-3P, Norte tipo Nord Lead, Uno tipo M1, Básicos). Oversampling 2x,
filtros de retardo cero con saturación por etapa y voicing por familia, chorus BBD del Juno con datos del manual,
envolventes analógicas, variación por nota, conversor de coma flotante en la FM. Diales: Densidad, Variación, Sub,
Glide, Corte, Reso, Env, Decay, Drive, Pump, Volumen, Swing, **Humano**. **Sub mono** (por debajo de 120 Hz).

**Exportar**: arrastrar MIDI y WAV (4 compases), **salida MIDI** en tiempo real (pendiente confirmar en Ableton).

**Presets de usuario**: GUARDAR / MIS PRESETS en `Documentos/Dias de Campo/Bass Maker Presets/*.ddcbass`.

**Interfaz**: ventana redimensionable (55 %–200 %), pantalla del campamento grande, logo DIAS DE CAMPO, pantallas
LED en 8 bits, piano roll y carril pixelados, panel MODULACIÓN, altavoz izquierdo que late (sin logos).

**Escena** (cada GENERAR avanza la noche hacia la mañana y sale alguien que se queda; 12 pasos y vuelve a empezar;
se guarda en el proyecto): gente bailando, fumando, bebiendo, sentada, saltando, durmiendo, parejas, tienda "ocupada";
DJ con cabina; hoguera, bombillas, luciérnagas, estrellas, luna, sol, pájaros. Cada 3 minutos, por turnos:
**avioneta** con cartel "OLD HERMIT CRAB X HESSA X ATMOODSPHERE" (0:10), **platillo volante** que abduce y devuelve
a alguien (1:10), **raver con jetpack** (2:10).

## 5. Pendiente

1. **Mac**: probar AU y VST3 en un Mac real.
2. **Salida MIDI**: confirmar si Ableton deja recogerla desde otra pista (MIDI From).
3. **Sonido con referencias**: grabar clips de un SH-101 / Juno-106 reales o de TAL-BassLine-101 / TAL-U-NO-LX
   (nota grave con barrido de filtro sin y con resonancia, chorus I y II, notas cortas) para afinar curvas.
4. **Certificar**: cuenta de desarrollador de Apple (firma + notarización) y certificado de firma de Windows;
   después adaptar `build.yml` con las claves como secretos de GitHub.
5. **Stab Maker**: primera versión nueva hecha (ver sección 7). Falta el **LINK** con el Bass Maker.
6. Para vender: revisar la **licencia de JUCE** y el uso de **marcas** (SH-101, Juno, DX7...: decir "inspirado en").
   Precio orientativo 39–49 € (lanzamiento 29 €), pack con el Stab Maker.

## 6. Notas técnicas útiles

- Interfaz a tamaño base **1330 × 800**, escalada con una transformación; el fondo estático se cachea en una imagen
  a la resolución real (por rendimiento en Windows/Direct2D). Lo animado usa imágenes de software reutilizadas.
- La escena se dibuja a 192 × 79 píxeles y se amplía ×3. Reloj de la escena: 15 fotogramas por segundo (`sceneT`).
- Oversampling: la voz trabaja a 2x (`os = 2`); todo lo que cuenta "por muestra" dentro de la voz usa `perSample`.
  La cuerda pulsada se calcula a 1x y se interpola.
- Estado guardado en el proyecto: patrones 0-7, carriles mood 0-7, `v2`, `edW` (ancho de ventana),
  `campStep` / `campSeeds` (campamento).

## 7. Stab Maker (proyecto hermano)

Carpeta propia (`dias-de-campo-stab-maker`), mismo código base que el Bass Maker. Clases `StabMakerProcessor` /
`StabMakerEditor`, código de plugin `Ddcs` (sustituye a la versión antigua).

- **Diseño**: sampler rectangular, cuerpo **teja**, tapa de arriba **gris oscuro** con letras crema, pantallas **verdes**,
  **8 pads de acordes** donde el Bass Maker tiene el altavoz, logo DIAS DE CAMPO en crema + "STAB MAKER".
- **Escena**: **lago al amanecer** (montañas, reflejo del sol/luna que brilla con los acordes, barca, embarcadero con
  pescador, bañista de día, tiendas, guirnalda, DJ y altavoz en la orilla). Mismo sistema de gente (12 sitios, 2 en el
  embarcadero), avioneta, platillo (también se lleva al de la barca) y jetpack cada 3 minutos.
- **Acordes**: `Source/Stab.h` (tipos de acorde dentro de la escala, nombres tipo Am7/Fmaj9, generador por estilos con
  capas de Densidad, pads con acordes del patrón + relleno con acordes de la escala, ajuste de volumen por sonido).
- **Sonido**: 8 voces del mismo motor (oversampling 2x), Ancho (reparto estéreo), Ataque, Delay ping-pong, Reverb,
  Pump, Drive (estos dos sin dial: los pone el estilo / el Mood). 19 sonidos. Arreglado el clic al soltar: la voz
  analógica ahora guarda su nivel (`amp = a * vel`) y la cola no se corta; en la FM la cola sigue a los operadores.
- **Herramientas del piano roll**: Poner/quitar, Acento, **Acorde** (cambia el tipo), Alargar, Octava.
- **Pruebas** (`test/Test.cpp`): patrones, niveles, `CLICKS`, `TAIL` (clic al soltar, detecta el fallo antiguo),
  `MEASURE`, `CPU` (~8–11 % en el peor caso), `EDIT`, `SHOT` (captura PNG de la interfaz).
- Arreglo compartido: el chorus podía leer una posición fuera del buffer por redondeo (también en el Bass Maker:
  `Synth.h` actualizado el 9/10/2026).
- **Pendiente**: LINK (tono/escala/progresión compartidos, pregunta-respuesta, generar a la vez, campamento compartido,
  la furgoneta del uno en la escena del otro).

## 8. Chord Maker (primera versión hecha el 9/10/2026)

- Mismo diseño que los otros dos (misma disposición, tapa oscura, logo + "CHORD MAKER").
- **Color elegido: AZUL** (cuerpo 0xff3e5f84, claro 0xff6f92ba, oscuro 0xff253c58, contorno 0xff0f1726);
  pantallas violeta oscuro (fondo 0xff160f2c, cielo del piano roll 0xff1a0f22 → 0xff47204a, rejilla rosa 0xffe0a8c8).
- Escena: **colina al atardecer** (árbol grande con guirnalda, mantas, guitarra, pareja, tiendas, DJ, avioneta).
- Columna izquierda **PROGRESIÓN** (8 acordes), herramienta **Inversión**, diales Release, Rasgueo y Arpegio.
- Maqueta: `chord_maker_prueba.png`.
- **Mejora pendiente del Stab Maker (y luego Chord Maker): grabar acordes en directo con los pads.** Botón REC; con la
  canción sonando, cada pad pulsado se escribe en el piano roll (cuantizado a 1/16, la duración = lo que se mantiene
  pulsado); modos Sumar / Reemplazar por vuelta; deshacer la toma entera; tras LIMPIAR los pads muestran los acordes de
  la escala; pads tocables con ratón, teclas 1-8 del ordenador y 8 notas MIDI fijas (C1-G1).
- El LINK queda aparcado por decisión del autor.

### Chord Maker: cómo está hecho
- Carpeta propia `dias-de-campo-chord-maker`, clases `ChordMakerProcessor` / `ChordMakerEditor`, código de plugin `Ddch`.
- `Source/Chord.h`: estilos de progresión (Deep house, Classic house, UK garage, Minimal, Dub, Lo-fi house, Rave 90s),
  números romanos, **enlace de voces** (posición cerrada o drop-2, la más cercana al acorde anterior; inversión a mano
  0..3; la octava de la raíz marca el registro), ritmos del modo Ritmo (cifras = nivel de Densidad), generador.
- Modos: **Pad / Ritmo / Arpegio** (`eventAt` lo usan la reproducción, el dibujo del piano roll y el MIDI exportado).
- Rasgueo (notas escalonadas alternando arriba/abajo), Release (multiplica las colas del sinte: `SynthParams::rel`),
  10 voces, **sin oversampling** (CPU ~6–10 % en el peor caso), reverb más grande.
- Columna PROGRESIÓN: clic = escuchar, clic derecho = menú con los acordes de la escala.
- Escena de la colina: el reloj va al revés (cada GENERAR baja el sol: tarde → atardecer → noche).
- 18 sonidos (pads, cuerdas, pianos, órganos, coro, cristal), volumen igualado (`chordPresetTrim`).

## 9. Drum Maker (primera versión hecha el 10/10/2026)

- Mismo diseño que los otros (logo + "DRUM MAKER"). **Color elegido: BURDEOS** (cuerpo 0xff7a2232, claro 0xffa8495a,
  oscuro 0xff4a1420, contorno 0xff1c070c), pantallas **azul noche** (fondo 0xff0c1626, carril de los interruptores
  0xff2a1016). Maqueta aprobada: `drum_maker_prueba.png`, escena `drum_maker_escena.png`.
- Escena: **carpa de baile de noche** (carpa de rayas, DJ dentro, altavoces que laten con el bombo, láseres, focos,
  guirnaldas, tiendas a los lados). La gente salta con cada bombo; cada GENERAR sale alguien y la noche avanza hasta
  el amanecer (reloj `tod = .72 · info.tod`). Avioneta, platillo (se lleva a alguien o al DJ) y jetpack cada 3 minutos.

### Drum Maker: cómo está hecho
- Carpeta propia `dias-de-campo-drum-maker`, clases `DrumMakerProcessor` / `DrumMakerEditor`, código de plugin `Ddcd`,
  categoría VST3 Instrument|Drum.
- `Source/DrumSynth.h`: síntesis (bombo seno con barrido, caja/palmas con ruido y filtros SVF, charles de 6 cuadradas
  tipo 808 + ruido, rim, tom, shaker, percusión con 4 variantes). 6 kits (Nueve 909, Ocho-Ocho 808, Siete, Minimal,
  Garage, Lo-fi). Niveles de charles y ride igualados entre kits.
- `Source/Drum.h`: `DrumStep` (vel, need = Densidad mínima, ratchet, nudge −3..3, prob), `DrumPattern` (10 filas × 64
  pasos + longitud por fila = polimetría), 8 estilos escritos como cadenas de 16 caracteres (0-3 nivel de Densidad,
  X acento, g fantasma, r/t redobles), generador y fills.
- Procesador: parámetros por fila (`lvl`, `pan`, `col`, `tune`, `dec` + número de fila), Tono (afina bombo y tom),
  Fills (No / Cada 4 / Cada 8), Groove (Recto / Shuffle / Suelto), Kit, compresión en paralelo, reverb por envío (el
  bombo seco), choke del charles, MIDI GM canal 10 (salida en tiempo real y archivo), entrada MIDI toca las filas.
  Protegido contra posiciones del host no válidas (ppq/bpm infinitos).
- Editor: la rejilla (clic en el nombre elige fila; clic derecho = menú longitud / rehacer / borrar / mute / solo;
  pintar arrastrando), herramientas Poner/quitar, Acento (clic derecho = fantasma), Redoble, Probab., Empuje; los
  diales de fila se reenganchan al cambiar de fila (`attachLaneKnobs`); pads con M y S.
- Pruebas (`test/Test.cpp`): patrones por estilo, niveles, `MEASURE` (cada instrumento en cada kit), `CPU` (~1 %),
  `EDIT`, `SHOT` (+ `SHOTLIVE=1` para capturar sonando). Pasa ASan/UBSan y compila con gcc y clang.
- **Pendiente**: grabar en directo con los pads (REC) también aquí; Lead Maker; LINK (aparcado).

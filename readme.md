# Problema de los N-Cuerpos y Colisiones

En este proyecto del ramo de Computación en GPU CC7515-1 Otoño 2024 se tuvieron 3 objetivos: poder realizar una visualización de los cuerpos interactuando, que esta sea producida totalmente en GPU (interoperabilidad con OpenCL) y que el mapeo en la API de cómputo en GPU fuera el aprendido en la lectura 3 (para mapeo a una matriz triangular).

El programa cuenta con:

- Cámara omnidireccional controlada con teclas y mouse
- Iluminación direccional; afecta la escena entera
- Cuerpos representados por una icósfera
- Una interfaz simple para el usuario
- Un kernel sin colisiones y otro con ellas

Se adelantan los resultados mencionando que solamente el mapeo en matriz triangular no se logró en la implementación por razones mencionadas posteriormente.

### Cámara de visualización y Respuesta al teclado y mouse

La cámara empieza en perspectiva mirando al centro de la escena. Para mover la cámara se pueden utilizar las teclas WASD. Para ascender se puede utilizar la tecla SPACE y para descender la tecla LEFT SHIFT.

El mouse permitirá cambiar hacia donde está mirando la cámara. Para rotar el ángulo de visión se debe colocar el cursor en los bordes del viewport (pero dentro de él). Además se puede hacer zoom con la rueda del mouse.

### Iluminación direccional/ambiental

Se tiene una luz que facilita la distinción de profundida en los objetos y que vuelve más realista la escena, mejorando la calidad de video. Se mantienen los colores de los cuerpos.

### Icósfera para los cuerpos

Un icosaedro es un poliedro regular de 20 caras, donde cada cara es un triángulo equilátero. Tiene 12 vértices y 30 aristas. En términos geométricos, es uno de los cinco sólidos platónicos.

Una icosfera es una aproximación a una esfera que se obtiene subdividiendo las caras de un icosaedro. Al subdividir repetidamente las caras triangulares del icosaedro y normalizar los nuevos puntos al radio deseado, se puede obtener una malla que se aproxima muy bien a una esfera.

Cuenta con una distribución uniforme de puntos al estar hecho de triángulos, permitiendo un renderizado eficiente.

### Interfaz simple para el usuario

Con Imgui se realizó una interfaz que muestra info de la escena:

- FPS
- Posición de la cámara
- Velocidad de la escena

### Kernel con/sin colisiones

Para actualizar los valores de posiciones y velocidades de los cuerpos se entregaban los buffer con la data a kernels de OpenCL. Dependiendo del modo elegido se calcula solo la atracción de los cuerpos o además se calcula la colisión.

En caso de tener colisiones se decidió no separar las actividades en 2 kernel pues así se aprovechan las iteraciones ya realizadas por el calculo de la atracción de los cuerpos. En caso de haber logrado el mapeo triangular se habrían separado las acciones.

Para la atracción y colisión de los cuerpos en el kernel, cada thread evalúa su interacción con los n-1 cuerpos restantes, acumulando los cambios y solo actualizando sus valores al final, sin peligro de dataraces.

## Complicaciones

Entre los problemas encontrados se encuentran los siguientes detalles:

- Diseño de la icósfera: se debe crear un icosaedro en primer lugar, al cual se le deben realizar subdivisiones por cara. Se debió controlar la forma de realizar las subdivisiones y encontrar la cantidad suficiente de ellas.

- Carga de los buffers con info: cada uno de los cuerpos ocupará la shape de la icósfera, por lo que no tiene sentido cargar más de una vez la forma de esta en GPU. Se descubrió el comando de dibujo por instancias en OpenGL.

- Aprendizaje sobre interoperabilidad: cómo inicializar OpenCL en un contexto de OpenGL, cómo conectar los buffers compartidos, cuándo realizar el traspaso de control sobre los buffers, etc. Fue aprendido sobre la marcha y en base a ensayo y error.

- Creación de los kernels: la parte de atracción de los cuerpos estaba en su mayoría adelantada por la tarea 2 del ramo. La parte de colisiones que se implementó requirió volver a investigar qué partes del código del kernel ya existente debía ser afectado. Para la matriz triangular se tuvo que estudiar en detalle la lectura, para luego pasar a implementarlo y visualizar múltiples fallos en la ejecución del programa, errores que no se supieron depurar y por los que se decidió desistir.

## Método de compilación y ejecución

Abrir una terminal en la carpeta principal del proyecto (nombre T3).

Para crear una build y compilar se deben presionar los siguientes comandos:

```bash
cmake -S . -B ./build
cmake --build ./build -j 10
```

Luego para ejecutar se debe dar el siguiente formato de comando:

```bash
./build/NBodyProblem.exe <n bodies> <local size> <group size> <radius> <subdivisions> <vel limit> <screen width> <screen height> <pos limit> <mode>
```

Tal que:

- n bodies: número de cuerpos en la simulación.
- local size: cantidad de threads por bloque
- group size: cantidad total de threads.
- radius: radio de la icósfera
- subdivisions: cantidad de veces que las caras de la icósfera se subdividen en 4.
- vel limit: velocidad límite inicial de los cuerpos
- screen with: número entero que representa ancho de la ventana que corre el programa.
- screen height: número entero que representa alto de la ventana que corre el programa.
- pos limit: en un inicio las partículas toman una posición aleatoria dentro de un cubo de lado 2\*"pos limit".
- mode: modo de ejecución. 1 significa con colisiones, de otra manera es sin colisiones.

Ejemplo de uso:

```bash
./build/NBodyProblem.exe 256 256 256 0.2 2 0.001 640 480 50 1
```

## Finalización del proyecto

Para mostrar el funcionamiento se adjunta un link a un video de la simulación con 64k cuerpos.

Con eso finalizaría este proyecto, gracias por la atención.

### Autores:

- Esteban López
- Sebastián Mira

internal void InitializeDefaultQuizItems(state *State)
{
#if 0
    /* 003_hacer_y_tener */
    AddQuizText("Qué tiempo (hacer)?", "hace");
    AddQuizText("En la mañana (hacer) buen tiempo en abril.", "hace");
    AddQuizText("En el verano (hacer) sol.", "hace");
    AddQuizText("Tú estudias español (hacer) una semana.", "hace");
    AddQuizText("Ella (hacer) una pregunta a su amigo.", "hace");
    AddQuizText("Él (hacer) una visita a su amigo.", "hace");
    AddQuizText("Él (hacer) una visita a Guatemala.", "hace");
    AddQuizText("Yo (hacer) un viaje a la playa.", "hago");
    AddQuizText("Nosotros (hacer) un viaje a las montañas.", "hacemos");

    AddQuizText("Yo (tener) 28 veintiocho años.", "tengo");
    AddQuizText("Yo (tener) poca sed.", "tengo");
    AddQuizText("Ella (tener) sed después de jugar.", "tiene");
    AddQuizText("Ella (tener) mucho frio.", "tiene");
    AddQuizText("El niño (tener) cuidado cuando él juega soccer.", "tiene");
    AddQuizText("Ellos (tener) prisa todos los días.", "tienen");
    AddQuizText("Yo no quiero (tener) prisa hoy.", "tener");
    AddQuizText("Después del trabajo yo (tener) mucho sueño.", "tengo");
    AddQuizText("La niña (tener) miedo de la oscuridad.", "tiene");
    AddQuizText("El niño no (tener) razón en la respuesta.", "tiene");
    AddQuizText("Yo (tener) razón en el argumento.", "tengo");
    AddQuizText("Ella (tener) suerte en el juego.", "tiene");
    AddQuizText("Él (tener) éxito en la vida.", "tiene");
    AddQuizText("Ellos (tener) ganas de comer una pizza.", "tienen");
    AddQuizText("Ella (tener) dolor de cabeza.", "tiene");
    AddQuizText("Ellos (tener) dolor de espalda.", "tienen");
    AddQuizText("Él (tener) que preparar la cena hoy", "tiene");

    { /* 002_estar_y_ser.txt */
        /* Ser */
        AddQuizText("Félix y Raúl (ser/estar) altos.", "son");
        AddQuizText("Antón (ser/estar) muy simpático.", "es");
        AddQuizText("Yo (ser/estar) Santiago.", "soy");
        AddQuizText("Este (ser/estar) el Teatro Real.", "es");
        AddQuizText("La familia de Carlos (ser/estar) católica.", "es");
        AddQuizText("Marta (ser/estar) de México DF.", "es");
        AddQuizText("Esa lámina (ser/estar) de Japón.", "es");
        AddQuizText("Tatiana y Sarai (ser/estar) mis hermanas.", "son");
        AddQuizText("Estos (ser/estar) mis amigos.", "son");
        AddQuizText("Elisa (ser/estar) mi exnovia.", "es");
        AddQuizText("Ese paraguas (ser/estar) mío.", "es");
        AddQuizText("El partido de fútbol (ser/estar) en Valencia.", "es");
        AddQuizText("El partido (ser/estar) el miércoles.", "es");
        AddQuizText("Hoy (ser/estar) domingo.", "es");
        AddQuizText("Hoy (ser/estar) 1 de abril.", "es");
        AddQuizText("En México ahora (ser/estar) por la mañana.", "es");
        AddQuizText("En el norte ya (ser/estar) de noche.", "es");
        AddQuizText("Las camas (ser/estar) hechas por Claudia.", "son");
        AddQuizText("Esa máquina (ser/estar) para Gabriel.", "es");
        AddQuizText("¿Cuánto es? (Ser/Estar) 120 euros.", "Son");
        AddQuizText("Ese anillo (ser/estar) de plata.", "es");
        AddQuizText("Ese anillo (ser/estar) de Perú.", "es");
        AddQuizText("Juana (ser/estar) ingeniera.", "es");
        AddQuizText("Ramón (ser/estar) periodista.", "es");
        /* Estar */
        AddQuizText("Yo (ser/estar) en paro.", "estoy");
        AddQuizText("(Ser/Estar) triste por el examen.", "Está");
        AddQuizText("Mi abuela (ser/estar) muy joven para su edad.", "está");
        AddQuizText("Alfredo (ser/estar) muy moreno.", "está");
        AddQuizText("Yo (ser/estar) soltero.", "estoy");
        AddQuizText("Yo (ser/estar) prometido.", "estoy");
        AddQuizText("Yo (ser/estar) casado.", "estoy");
        AddQuizText("Yo (ser/estar) divorciado.", "estoy");
        AddQuizText("Yo (ser/estar) viudo.", "soy");
        AddQuizText("El estadio (ser/estar) en Valencia.", "está");
        AddQuizText("Los jugadores (ser/estar) en el hotel.", "están");
        /* NOTE: Confused about the use of Estamos below... are these correct? */
        /* AddQuizText("(Ser/Estar) a domingo.", "Estamos"); */
        /* AddQuizText("(Ser/Estar) a 1 de abril.", "Estamos"); */
        /* AddQuizText("(Ser/Estar) en primavera.", "Estamos"); */
        AddQuizText("¿A qué día (ser/estar) hoy?", "estamos");
        AddQuizText("(Ser/Estar) bien irse de vacaciones una vez al año.", "Está");
        AddQuizText("Marco siempre (ser/estar) de buen humor.", "está");
        AddQuizText("Marisa (ser/estar) ahora de camarera en Ibiza.", "está");
        AddQuizText("Mi hermana (ser/estar) de parto.", "está");
        AddQuizText("Carla y Marina (ser/estar) de guardia este fin de semana.", "están");
        AddQuizText("¿A cuánto (ser/estar) las manzanas?", "están");
        AddQuizText("Todo el reloj (ser/estar) fabricado en oro.", "está");
        AddQuizText("El collar (ser/estar) hecho de papel.", "está");
        AddQuizText("El anillo (ser/estar) bañado en plata.", "está");
        AddQuizText("Nosotros (ser/estar) cenando en el jardín.", "estamos");
    }

    AddQuizText("_ tarea", "la");
    AddQuizText("_ restaurante", "el");
    AddQuizText("_ café", "el");
    AddQuizText("_ coche", "el");
    AddQuizText("_ viaje", "el");
    AddQuizText("_ parque", "el");
    AddQuizText("_ paquete", "el");
    AddQuizText("_ menú", "el");
    AddQuizText("_ colibrí", "el");
    AddQuizText("_ reloj", "el");
    AddQuizText("_ chocolate", "el");
    AddQuizText("_ pie", "el");
    AddQuizText("_ diente", "el");
    AddQuizText("_ noche", "la");
    AddQuizText("_ sangre", "la");
    AddQuizText("_ nube", "la");
    AddQuizText("_ llave", "la");
    AddQuizText("_ calle", "la");
    AddQuizText("_ gente", "la");
    AddQuizText("_ nieve", "la");
    AddQuizText("_ leche", "la");
    AddQuizText("_ carne", "la");
    AddQuizText("_ ley", "la");
    AddQuizText("_ imagen", "la");

    /* 006_gustar_homework */
    AddQuizText("A María no (me/te/le/nos/les gustar) este libro.", "le gusta");
    AddQuizText("A mí (me/te/le/nos/les gustar) mucho escribir composiciones.", "me gusta");
    AddQuizText("A ellos no (me/te/le/nos/les gustar) el clima de Alaska.", "les gusta");
    AddQuizText("A este joven (me/te/le/nos/les gustar) todas las muchachas del club.", "le gustan");
    AddQuizText("A los animales (me/te/le/nos/les gustar) la carne cruda.", "les gusta");
    AddQuizText("A Eneida no (me/te/le/nos/les gustar) los perros.", "le gustan");
    AddQuizText("A nosotros no (me/te/le/nos/les gustar) las ciudades que visitamos ayer.", "nos gustan");
    AddQuizText("A mis primas (me/te/le/nos/les gustar) mucho ir a los bailes.", "les gusta");
    AddQuizText("¿(me/te/le/nos/les gustar) a ti las clases de español?", "Te gustan");
    AddQuizText("A nosotros no (me/te/le/nos/les gustar) los exámenes.", "nos gustan");
    AddQuizText("A mí no (me/te/le/nos/les gustar) estudiar los sábados.", "me gusta");
    AddQuizText("A mis vecinos (me/te/le/nos/les gustar) mucho viajar.", "les gusta");
    AddQuizText("A nosotros no (me/te/le/nos/les gustar) los viajes muy largos.", "nos gustan");
    AddQuizText("Al maestro (me/te/le/nos/les gustar) escribir los ejercicios en la pizarra.", "le gusta");
    AddQuizText("¿(me/te/le/nos/les gustar) a Enrique trabajar en esa tienda?", "Le gusta");
    AddQuizText("A mi médico (me/te/le/nos/les gustar) escribir los ejercicios en la pizarra.", "le gusta");
    AddQuizText("A mí (me/te/le/nos/les gustar) mucho la película de anoche.", "me gusta");
    AddQuizText("A mi madre (me/te/le/nos/les gustar) sembrar flores.", "le gusta");
    AddQuizText("A mi no (me/te/le/nos/les gustar) el discurso del señor Gómez en la reunión de ayer.", "me gusta");
    AddQuizText("A ellos no (me/te/le/nos/les gustar) el paseo de la semana pasada.", "les gusta");

    /* 008_mas_verbos_homework EJERCICIO A (1 Singular) */
    AddQuizText("En la mañana yo (beber) café con leche.", "bebo");
    AddQuizText("Tú no (comer) carne.", "comes");
    AddQuizText("Nosotros (comprender) español.", "comprendemos");
    AddQuizText("Ellos (correr) en el parque.", "corren");
    AddQuizText("Ella (aprender) español en la Cooperativa.", "aprende");
    AddQuizText("Él (vender) limonada.", "vende");
    AddQuizText("Usted (leer) un nuevo libro.", "lee");
    AddQuizText("María no (beber) café.", "bebe");
    AddQuizText("Carlos y Anita (comer) una hamburguesa.", "comen");
    AddQuizText("Mi hermana (deber) estudiar más.", "debe");

    /* 008_mas_verbos_homework EJERCICIO A (2 Plurares) */
    AddQuizText("Yo (asistir) a la escuela todos los días.", "asisto");
    AddQuizText("Mis padres (vivir) en un pueblo pequeño.", "viven");
    AddQuizText("Yo (recibir) muchas cartas de los amigos.", "recibo");
    AddQuizText("Yo (escribir) a mis amigos.", "escribo");
    AddQuizText("Tú (abrir) las ventanas.", "abres");
    AddQuizText("El fin de semana nosotros (subir) las montañas.", "subimos");
    AddQuizText("Yo (compartir) mi casa con mi hermano.", "comparto");
    AddQuizText("Él (insistir) en viajar a Guatemala.", "insiste");
    AddQuizText("Yo (partir) la próxima semana.", "parto");
    AddQuizText("En Guatemala (existir) muchos lugares bonitos.", "existen");

    /* 007_verbos_homework EJERCICIO A */
    AddQuizText("Yo (hablar) español en la escuela.", "hablo");
    AddQuizText("En San Pedro yo no (necesitar) mucho dinero.", "necesito");
    AddQuizText("Yo (viajar) solo.", "viajo");
    AddQuizText("Tú (ayudar) a tu hermano.", "ayudas");
    AddQuizText("Tú (cocinar) mucha comida.", "cocinas");
    AddQuizText("Nosotros (visitar) muchas partes de Guatemala.", "visitamos");
    AddQuizText("En la tarde ellos (descansar) por una hora.", "descansan");
    AddQuizText("Cuando yo (tomar) cerveza, yo (fumar) mucho.", "tomo fumo");
    AddQuizText("Él (regresar) a su país la próxima semana.", "regresa");
    AddQuizText("Ella (lavar) su ropa cada semana.", "lava");
    AddQuizText("Yo (pagar) veinticinco quetzales por una comida.", "pago");
    AddQuizText("Ellos (desear) estudiar español.", "desean");
    AddQuizText("Yo (estudiar) español en la Cooperativa.", "estudio");
    AddQuizText("Mi padre (trabajar) ocho horas cada día.", "trabaja");
    AddQuizText("Yo (mirar) una película cada semana en la escuela.", "miro");

    /* 009_y_mas_verbos_homework */
    AddQuizText("Yo (salir) a las doce de la escuela.", "salgo");
    AddQuizText("Yo (hacer) la tarea todas las tardes.", "hago");
    AddQuizText("Yo (traer) libros a la escuela.", "traigo");
    AddQuizText("Yo (poner) mis libros en la mesa.", "pongo");
    AddQuizText("Yo (saber) hablar poco español.", "sé");
    AddQuizText("Yo (dar) la tarea al profesor.", "doy");
    AddQuizText("Yo (hacer) ejercicios físicos.", "hago");
    AddQuizText("Yo (ver) un programa interesante.", "veo");
    AddQuizText("Yo no (salir) en este espacio.", "salgo");
    AddQuizText("Yo (salir) la próxima semana.", "salgo");
    AddQuizText("¿Qué tipo de música (oír) tú?", "oyes");
    AddQuizText("Yo (venir) a la escuela.", "vengo");
    AddQuizText("Cuando yo salgo de la casa (decir) adiós.", "digo");
    AddQuizText("Yo (oír) las noticias en la radio.", "oigo");
    AddQuizText("Mis amigos no (venir) a la escuela hoy.", "vienen");
    AddQuizText("¿Qué (decir) tú cuando recibes un regalo?", "dices");
    AddQuizText("Yo (decir) gracias.", "digo");
    AddQuizText("Nosotros (venir) temprano a la escuela.", "venimos");
    AddQuizText("Cuando él (venir) a la escuela, siempre (oír) música.", "viene oye");
    AddQuizText("Ellos (oír) el pronóstico del tiempo.", "oyen");
    AddQuizText("Cerca de mi casa yo (oír) mucho ruido.", "oigo");
    AddQuizText("¿A qué hora (venir) tú a la casa?", "vienes");
    AddQuizText("Mis amigos y yo siempre (decir) la verdad.", "decimos");
    AddQuizText("Cuando yo estoy en el centro del pueblo (oír) mucho ruido.", "oigo");
    AddQuizText("El maestro (oír) mis respuestas con atención.", "oye");
    AddQuizText("Yo (poder) hablar alemán.", "puedo");
    AddQuizText("Nosotros no (recordar) mucho.", "recordamos");
    AddQuizText("Mi hermano (dormir) ocho horas cada día.", "duerme");
    AddQuizText("¿Dónde (almorzar) tú normalmente?", "almuerzas");
    AddQuizText("Yo (volver) a las cuatro del trabajo.", "vuelvo");
    AddQuizText("Mi mejor amigo (jugar) fútbol.", "juega");
    AddQuizText("¿Te (acordar) tú de la fecha de la fiesta?", "acuerdas");
    AddQuizText("¿Cuántas horas (volar) tú desde Europa hasta América?", "vuelas");
    AddQuizText("¿A qué hora (almorzar) tu familia normalmente?", "almuerza");
    AddQuizText("Los estudiantes (devolver) los libros a la escuela.", "devuelven");
    AddQuizText("¿Cuánto (costar) una cámara digital?", "cuesta");
    AddQuizText("Mi abuelo (contar) sus experiencias.", "cuenta");
    AddQuizText("Muchas plantas (morir) en verano.", "mueren");
    AddQuizText("Nosotros (mostrar) las fotos a nuestros amigos.", "mostramos");
    AddQuizText("En invierno (llover) mucho.", "llueve");
#endif

#if 0
    /* 010_mas_mas_verbos_homework.txt */
    AddQuizText("Yo (querer) visitar tu país.", "quiero");
    AddQuizText("Nosotros (empezar) las clases a las ocho de la mañana.", "empezamos");
    AddQuizText("Ella (perder) sus llaves frecuentemente.", "pierde");
    AddQuizText("Ellas (entender) el español muy bien.", "entienden");
    AddQuizText("Mi hermano se (despertar) muy tarde.", "despierta");
    AddQuizText("Mi padre (cerrar) la puerta.", "cierra");
    AddQuizText("En el cine yo me (sentar) en la tercera fila.", "siento");
    AddQuizText("¿Cuándo (empezar) el invierno en tu país?", "empieza");
    AddQuizText("Mi padre (defender) a su familia.", "defiende");
    AddQuizText("Nosotros (confesar) la verdad a mi madre.", "confesamos");
    AddQuizText("En Europa (nevar) mucho.", "nieva");
    AddQuizText("En Guatemala los ricos (gobernar) siempre.", "gobiernan");
    AddQuizText("Ellos (querer) aprender español.", "quieren");
    AddQuizText("Yo (entender) tu problema.", "entiendo");
    AddQuizText("¿Quién (calentar) el agua para el té en tu casa?", "calienta");
#endif

#if 0
    /* 011_VERBOS_homework.txt */
    AddQuizText("Los ladrones (huir) de la policía.", "huyen");
    AddQuizText("La próxima semana (concluir) las clases.", "concluyen");
    AddQuizText("Nosotros (incluir) a Juan en nuestro equipo.", "incluimos");
    AddQuizText("Mi padre (construir) una casa muy bonita.", "construye");
    AddQuizText("Yo (contribuir) con los jóvenes.", "contribuyo");
    AddQuizText("Mario (distribuir) dulces a los niños.", "distribuye");
    AddQuizText("Mis hermanos (distinguir) las cosas buenas y malas.", "distinguen");
    AddQuizText("Mi hijo (destruir) sus juguetes.", "destruye");
    AddQuizText("Los bomberos (extinguir) el fuego.", "extinguen");
    AddQuizText("Los productos de Estados Unidos (influir) en muchos países.", "influyen");
    AddQuizText("Ella nunca me (incluir) entre sus invitados.", "incluye");
    AddQuizText("Ese gato siempre (destruir) las flores de nuestro jardín.", "destruye");
    AddQuizText("Ese gato siempre (huir) cuando mi perro está en el jardín.", "huye");
    AddQuizText("Nosotros (contribuir) con el desarrollo de nuestro pueblo.", "contribuimos");
    AddQuizText("Yo no (distinguir) nada de lejos.", "distingo");
    AddQuizText("Para ir a la escuela Cooperativa los estudiantes siempre (seguir) este camino.", "siguen");
    AddQuizText("Yo siempre (conseguir) lo que quiero.", "consigo");
    AddQuizText("La policía (perseguir) a los criminales.", "persigue");
    AddQuizText("El profesor (concluir) sus clases con una broma.", "concluye");
    AddQuizText("Cuando la gente (construir) una casa, necesita mucho dinero.", "construye");
#endif

#if 0
    /* 014_preterito_tarea.txt */
    AddQuizText("La semana pasada yo (estudiar) cuatro horas cada día.", "estudié");
    AddQuizText("Yo (llegar) a San Pedro la semana pasada.", "llegué");
    AddQuizText("Mis amigos (visitar) San Pedro el año pasado.", "visitaron");
    AddQuizText("Yo (colocar) mi ropa en el armario ayer.", "colozcé");
    AddQuizText("Anoche mis amigos y yo (mirar) la película.", "miramos");
    AddQuizText("Ella (visitar) Honduras el año pasado.", "visitó");
    AddQuizText("Ustedes (tomar) muchas cervezas anoche.", "tomaron");
    AddQuizText("Yo (buscar) a mis amigos en el bar anoche.", "buscé");
    AddQuizText("El fin de semana nosotros (nadar) en el lago.", "nadamos");
    AddQuizText("El domingo ella (comprar) muchos regalos en Chichicastenango.", "compró");
    AddQuizText("Yo (empezar) a estudiar en la Cooperativa hace dos semanas.", "empezé");
    AddQuizText("La semana pasada yo (jugar) con los niños.", "jugué");
    AddQuizText("Anoche durante la cena yo (hablar) mucho con mi familia.", "hablé");
    AddQuizText("El jueves pasado los estudiantes de la Cooperativa (bailar) salsa.", "bailaron");
    AddQuizText("El fin de semana pasado yo no (practicar) mucho mi español.", "practicé");
#endif

#if 0
    /* 015_irregular_preterito_tarea.txt */
    AddQuizText("Mi amigo (oír) música clásica toda la noche.", "oyó");
    AddQuizText("Ella (leer) una novela el fin de semana.", "leió");
    AddQuizText("Mi gato (huir) de los perros anoche.", "huió");
    AddQuizText("Los niños no (creer) la historia.", "creyeron");
    AddQuizText("La semana pasada un avión (caer) en el lago.", "cayó");
    AddQuizText("Mis padres (construir) esta casa hace diez años.", "construyeron");
    AddQuizText("En el año 1976 un terremoto (destruir) muchas casas en Guatemala.", "destruyó");
    AddQuizText("Después del huracán Stan mucha gente (contribuir) con los damnificados.", "contribuyeron");
    AddQuizText("Los estudiantes (leer) su tarea en la escuela.", "leyeron");
    AddQuizText("El año pasado una bomba (caer) en el centro del pueblo.", "cayó");
    AddQuizText("La explosión se (oír) hasta Panajachel.", "oyó");
    AddQuizText("Esa bomba (destruir) el mercado.", "destruyó");
    AddQuizText("El fin de semana mis amigos (oír) música latina.", "oyeron");
    AddQuizText("Los bandidos (huir) de la policía.", "huyeron");
    AddQuizText("Mis hermanos (construir) una nueva casa.", "construyeron");
    AddQuizText("Mi hermana (ir) a Europa el año pasado.", "fue");
    AddQuizText("Yo le (dar) un regalo a mi padre el día de su cumpleaños.", "di");
    AddQuizText("Yo no (ver) a otros estudiantes en la fiesta anoche.", "vi");
    AddQuizText("Mi hermano (ser) un buen estudiante de español en el pasado.", "fue");
    AddQuizText("Ellos (ir) a la playa el fin de semana.", "fueron");
    AddQuizText("Hace un semana, después de comer en el restaurante ellos (dar) una propina al mesero.", "dieron");
#endif

#if 0
    /* 018_homework.txt */
    AddQuizText("Él (hablar) con su madre ayer.", "habló");
    AddQuizText("Yo (comer) mucho la semana pasada.", "comí");
    AddQuizText("Mis padres (vivir) en florida hace cuarenta años.", "vivieron");
    AddQuizText("Nosotros (nadar) en el lago ayer.", "nadamos");
    AddQuizText("Yo (tener) mucho frio.", "tuve");
    AddQuizText("Tú (leer) muchos libros el año pasado.", "leíste");
    AddQuizText("Ellos (construir) esta casa hace 80 años.", "construyeron");
    AddQuizText("Ella (conducir) al trabajo ayer.", "condujo");
    AddQuizText("Él (hacer) muchas cosas esta semana.", "hizo");
    AddQuizText("Yo no (poder) trabajar hace algunos años.", "pude");
#endif

#if 1
    char *Foo[Max_Variable_Answers][Max_Variables] = {{"ahí"}, {"allí"}};
    AddQuizItemVariableAnswer(
        A({"Llevo esta blusa porque ____ de ", " no va bien con aquella falda."}),
        Foo,
        A({"ésa", "aquella"}),
        3, 2);
#else
No llevo ese chaleco porque (éste) de aquí es más bonito.
Estos zapatos son más cómodos que (aquellos) de allí.
Voy a llevar aquel traje de baño porque es más nuevo que (éste) de aquí.
Esos pantalones cortos son más prácticos que (aquellos) de allí.
Aquellas sandalias son más de moda que (ésas) de ahí.
Estos suéteres son más ligeros que (aquellos) de allí.
Esa falda negra es más útil que (esta) de aquí de flores.
La única bolsa que voy a llevar es (esa) de ahí.
#endif

#if 0
    /* Some word translations */
    AddQuizText("enero", "January");
    AddQuizText("febrero", "February");
    AddQuizText("marzo", "March");
    AddQuizText("abril", "April");
    AddQuizText("mayo", "May");
    AddQuizText("junio", "June");
    AddQuizText("julio", "July");
    AddQuizText("agosto", "August");
    AddQuizText("septiembre", "September");
    AddQuizText("octubre", "October");
    AddQuizText("noviembre", "November");
    AddQuizText("diciembre", "December");
    AddQuizText("abrir", "to open");
    AddQuizText("abuelo", "grandfather");
    AddQuizText("abuela", "grandmother");
    AddQuizText("acento", "accent");
    AddQuizText("acordar", "to agree");
    AddQuizText("acordeón", "accordion");
#endif

#if 0
    AddQuizText("la actividad", "activity");
    AddQuizText("las actividades", "activities");
    AddQuizText("actualmente", "currently"); /* nowadays */
    AddQuizText("adjetivo", "adjective");
    AddQuizText("adjetivos", "adjectives");
    AddQuizText("adverbio", "adverb");
    AddQuizText("adverbios", "adverbs");
    AddQuizText("afuera", "outside");
    AddQuizText("el/la agente", "agent");
    AddQuizText("agradecer", "to thank");
    AddQuizText("agregar", "to gather");
    AddQuizText("el agricultor", "farmer");
    AddQuizText("la agricultora", "farmer");
    AddQuizText("los agricultores", "farmers");
    AddQuizText("el agua", "water");
    AddQuizText("ahora", "now");
    AddQuizText("alergia", "allergy"); /* ALSO: allergic, allergies */
    AddQuizText("algo", "something"); /* ALSO: anything */
    AddQuizText("alguno", "any"); /* ALSO: one */
    AddQuizText("algunos", "some"); /* ALSO: few */
#endif
}
#if 0 /* TODO: delete these words after boiling them down to translatable words */
LA/S mano/s
almorzar
alrededor
alto
alumno
amable
ambiguo
amigo
anillo
animal
animales
ánimo
año
anoche
año
antes
apagada
aparecer
apariencia
apartamento
aprender
apropiada
aquí
argumento
arroz
artista
asistir
aspecto
asunto
atención
autobús
avión
ayer
ayudar
azar
bailar
bañado
barco
basquetbol
basura
bebé
beber
bebida
bicicleta
bien
blanco
bomberos
bonita
bosque
broma
buen
buscar
caballo
cabeza
cada
caer
café
calentar
calienta
calle
calor
cámara
camarera
camas
cambia/o
cambiar
caminar
canción
cansado
capitán
carne
caro
carpintero
carro
carta
casa
casado
católica
cenar
cer
cerca
cero
cerrar
cerveza
chocolate
cien
cinco
cincuenta
cine
ciudad/es
civil
clarificar
clase/s
clásica
cliente
clima
club
coche
cocinar
cocinero
collar
colobrí
comenzar
comer
comida
compañía
compartir
composiciones
comprar
comprender
computadora/s
con
concluir
condicional
conducir
conferencia
confesamos
confesar
confusión
conjugación
conocer
conseguir
consonante
construir
contar
contestar
contexto
contribuir
cooperativa
corbatas
cortesía
correctamente
corregir
correr
corresponde
corrupto
cosa/s
costar
crecer
creer
crema
criminal/es
crisis
cruz/cruces
cruda
cuadernos
cuál
cuando
cuándo
cuánto/s
cuarenta
cuarto
cuatro
cubrir
cucharita
cuenta
cuesta
cuidado
cumpleaños
cuya/s
cuyo/s
dar
de
deber
decidir
decir
defender
definición
definido
del
dentista
depender
deportista
derechos
des
desaparecer
desarrollar
desayunar
descansar
describir
descubrir
desde
desear
despertar
después
destinatario
destruir
devolver
día/s
diecinueve
dieciocho
dieciséis
diecisiete
diente
diez
diferente
dificultad
digital
dinero
dirección
dirigir
discurso
distancia
distinguir
distribuir
dividir
divorciado
doce
dolor
domingo
dónde
dormir
dos
dulce/s
durante
ejemplo/s
ejercicio/s
elección
elegir
eliminar
empezar
encender
encontrar
enfermo
enojar
enseñar
entender
entre
equipo
esa
escoger
esconder
escribir
escuchar
escuela
ese
espacio/s
espalda
esperar
estación
estaciones
estadio
estado/s
estar
este
estos
estructura
estudiante/s
estudiar
estuve
estuvieron
evento
evitar
examen/exámenes
excepcion/es
excepto
exigir
existir
éxito
novia/o
experiencia/s
explicación
expresar
expresión/expresiones
extinguir
extraña
fabricado
fácilmente
falda
familia/s
familiar
febrero
fecha
femenino/a
fiesta
fila
fin
finales
finalidad
fingir
físico/a
físicos
flor/es
fogata
forma
formule
foto/s
frase/s
frecuentemente
fresca
frio/s
fruta
fuego
fumar
fútbol
futuro
ganar
gato/s
general
generalmente
gente
gerente
gerundio
gobernar
gorriones
gracias
grama
grande
grupo
guante/s
guardia
gustar
hablar
hacer
hambre
hambriento
hamburguesa
hasta
hecho/a
hermano/a
hija/o
historia
hola
hombre
hora/s
horno
hoy
huir
humor
identidad
identificar
identificativos
idiomas
imagen
impedir
imperfecto
incluir
indefinido
indicativo
infinitive
infinitivo
influir
información
ingeniera
insistir
interesante
interrogativa/s
invariable/s
invierno
invitados
ir
irregulares
irse
jardín
jefe
joven/jóvenes
juego/s
jueves
jugadores
jugar
juguete/s
junto/s
kilo
la
ladron/es
lago
lámina
lámpara
lápiz/lapices
largo/s
lavar
lección
leche
leer
legumbre
lejos
ley
libro/s
limonada
llamar
llave/s
llegar
llover
localización
lógica/s
luego
lugar
lugar/es
lunes
madre
maestro/s
maíz
mal
malos/as
mamá
mañana/s
mano/s
manzanas
mapa
máquina
martes
más
mascota
masculino/a
materia
mediar
medicina
médico
medir
mejor
menos
mentales
mentir
menú
menudo
mes
mesa
mesero/a
miel
miércoles
minuto/s
mío
mirar
mis
modal/es
modo
momento
montañas
morder
moreno
morir
mostrar
moto
movimiento
mucho/a
muchacho/a
muchos/as
muelle
mujer
mundo
muñeca
música
nada
nariz
naturaleza
necesitar
negrilla
nevar
nieve
niña
ninguno/a
niño/a
noche/s
nombre
normal
normalmente
norte
nos
nosotros
nota
noticia/s
noventa
noviembre
nube
nuestro/a
nuevo/a
nueve
nunca
obedecer
objetivo
objeto
ochenta
ocho
octubre
oficina
ofrecer
oír
oler
oracion/es
ordenar
origen
orar
orquesta
oscuridad
otro/a
paciencia
padre/s
pagar
país/es
pájaro/s
palabra/s
pan
papá
papa/s
papel/es
paquete
paraguas
paréntesis
paro
parque
parte/s
participar
participio
partido
partir
pasada
pasar
paseo
pasta
pastel
pausa
pec/es
pedir
película
pelota
penoso/a
pensar
pequeño
perder
periodista
permiso
permitir
pero
perro/s
perseguir
persona/s
personalidad
pertenencia
pez
pie
piel
pizarra
planeta
plantas
plata
playa
plural/es
pobre/s
poca/o
poder
policía/s
pollo
poner
porque
posesión
posesivo/s
posible
practicar
precio
preferir
pregunta/s
preparar
preposición/es
presente
presidente
pretender
primo/a
primavera
primero/a
prisa
probar
problema
producción
producir
producto/s
profesión
profesor/es
programar
prometer
prometido
pronóstico
proseguir
proteger
próxima
pueblo
puerta/s
punto
querer
quince
radio
rasgos
razón
recibir
recoger
reconocer
recordar
reducir
referir
regalo
regla/s
regresar
regular/es
reír
reírse
relacion/es
reloj
repetir
responder
reunión
revisar
rey
rico/s
rio
rompecabeza/s
ropa
ruido
sábado
saber
salir
salsa
salud
san
sangre
sed
seguir
seis
selección
semana
sembrar
señor
sentar
sentir
ser
servir
sesenta
setenta
siempre
siguiente/s
silaba/s
simpático
singular/es
sofá
sol
solamente
solo/a
soltero
sonar
soñar
sonreír
subir
sueño
suerte
sufrir
sugerir
sujeto/s
sustantivo/s
sustituir
también
tarde/s
tarea/s
teatro
televisión
temprano
tener
tercera
terminar
tiempo
tienda
tipo
tocar
todo/a/os/as
tomar
toro
trabajar
traducir
traer
trece
treinta
tren
tres
triste
trompeta
turista
ubicación
uir
última
usar
utilizar
vacacion/es
vaso/s
vez/veces
vecino/s
veinte
vender
venir
ventana/s
ver
verbo/s
verdad
verde
verduras
vergonzoso
vergüenza
viajar
vida
viento
viernes
visitar
viudo
vivir
vocal
vocálicos
volar
volcán
volver
yegua
zumo/s
#endif

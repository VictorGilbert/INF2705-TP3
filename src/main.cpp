// Prénoms, noms et matricule des membres de l'équipe:
// - Majeed Abdul Baki (2147602)
// - Victor Gilbert (2143708)
//#pragma message (": *************** Identifiez les membres de l'équipe dans le fichier 'main.cpp' et commentez cette ligne. ***************")

#if defined(_WIN32) || defined(WIN32)
#pragma warning ( disable : 4244 4305 )
// warning C4244: 'conversion' conversion from 'double' to 'float', possible loss of data
// warning C4305: 'argument' : truncation from 'double' to 'float'
#endif

#include <stdlib.h>
#include <iostream>
#include "inf2705-matrice.h"
#include "inf2705-nuanceur.h"
#include "inf2705-fenetre.h"
#include "inf2705-texture.h"
#include "inf2705-forme.h"
#include "Etat.h"
#include "Pipeline.h"
#include "Camera.h"

// les formes
FormeSphere *sphere = NULL, *sphereLumi = NULL;
FormeTheiere *theiere = NULL;
FormeTore *tore = NULL;
FormeCylindre *cylindre = NULL;

// partie 2: texture
const int texCoulNum = 5;
const int texNormNum = 5;
GLuint texturesCoul[texCoulNum]; // les textures de couleurs chargées
GLuint texturesNorm[texNormNum]; // les textures de normales chargées
GLuint heightMapTex;

////////////////////////////////////////
// déclaration des variables globales //
////////////////////////////////////////

// définition des lumières
glm::vec4 posLumiInit[3] = { glm::vec4( -7.3, -6.8,  5.0, 1.0 ),
                             glm::vec4(  7.5,  5.5,  14.0, 1.0 ),
                             glm::vec4(  0.3,  5.8, -12.0, 1.0 ),
};
glm::vec4 spotDirInit[3] = { 4.0f*glm::normalize( glm::vec4( 2.5, 2.6, -1.6, 1.0 ) ),
                             4.0f*glm::normalize( glm::vec4( -1.6, -1.2, -3.4, 1.0 ) ),
                             4.0f*glm::normalize( glm::vec4( -0.2, -1.6, 3.6, 1.0 ) ) };
struct LightSourceParameters
{
    glm::vec4 ambient[3];
    glm::vec4 diffuse[3];
    glm::vec4 specular[3];
    glm::vec4 position[3];       // dans le repère du monde (il faudra convertir vers le repère de la caméra pour les calculs)
    glm::vec4 spotDirection[3];  // dans le repère du monde (il faudra convertir vers le repère de la caméra pour les calculs)
    float spotExposant;
    float spotAngleOuverture;    // angle d'ouverture delta du spot ([0.0,90.0] ou 180.0)
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
} LightSource = { { glm::vec4( 0.3, 0.1, 0.1, 1.0 ),
                    glm::vec4( 0.1, 0.3, 0.1, 1.0 ),
                    glm::vec4( 0.1, 0.1, 0.3, 1.0 ) },
                  { glm::vec4( 1.0, 0.3, 0.3, 1.0 ),
                    glm::vec4( 0.3, 1.0, 0.3, 1.0 ),
                    glm::vec4( 0.3, 0.3, 1.0, 1.0 ) },
                  { glm::vec4( 1.0, 0.5, 0.5, 1.0 ),
                    glm::vec4( 0.5, 1.0, 0.5, 1.0 ),
                    glm::vec4( 0.5, 0.5, 1.0, 1.0 ) },
                  { posLumiInit[0], posLumiInit[1], posLumiInit[2] },
                  { spotDirInit[0], spotDirInit[1], spotDirInit[2] },
                  1.0,       // l'exposant du cône
                  20.0,      // l'angle du cône du spot
                  1., 0., 0. // atténuation
};

// définition du matériau
struct MaterialParameters
{
    glm::vec4 emission;
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
    float shininess;
} FrontMaterial = { glm::vec4( 0.0, 0.0, 0.0, 1.0 ),
                    glm::vec4( 0.3, 0.3, 0.3, 1.0 ),
                    glm::vec4( 0.8, 0.8, 0.8, 1.0 ),
                    glm::vec4( 1.0, 1.0, 1.0, 1.0 ),
                    90.0 };

struct LightModelParameters
{
    glm::vec4 ambient; // couleur ambiante globale
    int twoSide;       // éclairage sur les deux côtés ou un seul?
} LightModel = { glm::vec4( 0.2, 0.2, 0.2, 1.0 ), false };

struct
{
    // partie 1: illumination
    int typeIllumination;     // 0:Gouraud, 1:Phong
    int utiliseSpot;          // indique si on veut utiliser modèle ponctuel ou spot
    int utiliseBlinn;         // indique si on veut utiliser modèle spéculaire de Blinn ou Phong
    int utiliseDirect;        // indique si on utilise un spot style Direct3D ou OpenGL
    int afficheNormales;      // indique si on utilise les normales comme couleurs (utile pour le débogage)
    // partie 2: texture
    float tempsGlissement;    // temps de glissement (=0.1*Etat::temps)
    int iTexCoul;             // numéro de la texture de couleurs appliquée
    // partie 3b: texture
    int iTexNorm;             // numéro de la texture de normales appliquée
} varsUnif = { 0, 0, 0, 0, 0,
               0.0, 0, 0 };
// ( En GLSL, les types 'bool' et 'int' sont de la même taille, ce qui n'est pas le cas en C++.
// Ci-dessus, on triche donc un peu en déclarant les 'bool' comme des 'int', mais ça facilite la
// copie directe vers le nuanceur où les variables seront bien de type 'bool'. )


void calculerPhysique( )
{
    // ajuster le dt selon la fréquence d'affichage
    {
        static int tempsPrec = 0;
        // obtenir le temps depuis l'initialisation (en millisecondes)
        int tempsCour = FenetreTP::obtenirTemps();
        // calculer un nouveau dt (sauf la première fois)
        if ( tempsPrec ) Etat::dt = ( tempsCour - tempsPrec )/1000.0;
        // se préparer pour la prochaine fois
        tempsPrec = tempsCour;
    }

    if ( Etat::englissement )
    {
        // avancer le temps
        Etat::temps += Etat::dt;
        varsUnif.tempsGlissement = 0.1 * Etat::temps; // facteur de 0.1 pour ne pas que ça soit trop rapide
    }

    if ( Etat::enmouvement )
    {
        // déplacer les lumières
        {
            glm::mat4 mat = glm::rotate( glm::mat4(1.0), GLfloat(0.01), glm::vec3(0,1,0) );
            LightSource.position[0] = mat * LightSource.position[0];
            LightSource.position[1] = mat * LightSource.position[1];
            LightSource.position[2] = mat * LightSource.position[2];
            LightSource.spotDirection[0] = mat * LightSource.spotDirection[0];
            LightSource.spotDirection[1] = mat * LightSource.spotDirection[1];
            LightSource.spotDirection[2] = mat * LightSource.spotDirection[2];
        }
        // faire varier un peu la brillance
        {
            static int sensS = 1;
            static double base = 2.0;
            base += sensS * 0.01;
            if ( base < 1.5 ) sensS = +1;
            else if ( base > 2.5 ) sensS = -1;
            FrontMaterial.shininess = pow(10,base);
        }

#if 0
        static int sensAngle = 1;
        LightSource.spotAngleOuverture += sensAngle * 15.0 * Etat::dt;
        if ( LightSource.spotAngleOuverture < 5.0 ) sensAngle = -sensAngle;
        if ( LightSource.spotAngleOuverture > 60.0 ) sensAngle = -sensAngle;
#endif

        if ( getenv("DEMO") != NULL )
        {
            // De temps à autre, alterner entre le modèle d'illumination: Gouraud, Phong
            static float type = 0;
            type += 0.005;
            varsUnif.typeIllumination = fmod(type,2);
        }
    }

    camera.verifierAngles();
}

void charger1Texture( std::string fichier, GLuint &texture )
{
    unsigned char *pixels;
    GLsizei largeur, hauteur;
    if ( ( pixels = ChargerImage( fichier, largeur, hauteur ) ) != NULL )
    {
        glGenTextures( 1, &texture );
        glBindTexture( GL_TEXTURE_2D, texture );
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, largeur, hauteur, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glGenerateMipmap( GL_TEXTURE_2D );
        glBindTexture( GL_TEXTURE_2D, 0 );
        delete[] pixels;
    }
}
void chargerTextures()
{
    glActiveTexture( GL_TEXTURE0 ); // l'unité de texture 0
    charger1Texture( "textures/terre.bmp", texturesCoul[0] );
    charger1Texture( "textures/echiquier.bmp", texturesCoul[1] );
    charger1Texture( "textures/mur.bmp", texturesCoul[2] );
    charger1Texture( "textures/metal.bmp", texturesCoul[3] );
    charger1Texture( "textures/mosaique.bmp", texturesCoul[4] );
    charger1Texture( "textures/heightMap.bmp", heightMapTex);
    glActiveTexture( GL_TEXTURE1 ); // l'unité de texture 1
    charger1Texture( "textures/pierresNorm.bmp", texturesNorm[0] );
    charger1Texture( "textures/bullesNorm.bmp", texturesNorm[1] );
    charger1Texture( "textures/murNorm.bmp", texturesNorm[2] );
    charger1Texture( "textures/briqueNorm.bmp", texturesNorm[3] );
    charger1Texture( "textures/circuitNorm.bmp", texturesNorm[4] );
}

void chargerNuanceur(GLuint progId, GLenum type, const GLchar** code)
{
	GLuint nuanceurObj = glCreateShader( type );
	glShaderSource( nuanceurObj, 1, code, NULL );
	glCompileShader( nuanceurObj );
	glAttachShader( progId, nuanceurObj );
	ProgNuanceur::afficherLogCompile( nuanceurObj );
}

void chargerProgNuanceurs()
{
    // charger le nuanceur de base
    {
        // créer le programme
        progBase = glCreateProgram();

        // attacher le nuanceur de sommets
		chargerNuanceur(progBase, GL_VERTEX_SHADER, &ProgNuanceur::chainesSommetsBase);
        // attacher le nuanceur de fragments
		chargerNuanceur(progBase, GL_FRAGMENT_SHADER, &ProgNuanceur::chainesFragmentsBase);

        // faire l'édition des liens du programme
        glLinkProgram( progBase );
        ProgNuanceur::afficherLogLink( progBase );

        // demander la "Location" des variables
        if ( ( locVertexBase = glGetAttribLocation( progBase, "Vertex" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de Vertex" << std::endl;
        if ( ( locColorBase = glGetAttribLocation( progBase, "Color" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de Color" << std::endl;
        if ( ( locmatrModelBase = glGetUniformLocation( progBase, "matrModel" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de matrModel" << std::endl;
        if ( ( locmatrVisuBase = glGetUniformLocation( progBase, "matrVisu" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de matrVisu" << std::endl;
        if ( ( locmatrProjBase = glGetUniformLocation( progBase, "matrProj" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de matrProj" << std::endl;
    }

    if (prog != 0)
    {
        glUseProgram(0);
        glDeleteProgram(prog);
    }

    // charger le nuanceur de ce TP
    if ( Etat::utiliseTess ) 
    {
        // créer le programme
        prog = glCreateProgram();

        // attacher le nuanceur de sommets
        const GLchar *chainesSommets = ProgNuanceur::lireNuanceur( "./nuanceurs/tess.sommets.glsl" );		
        if ( chainesSommets != NULL )
        {
			chargerNuanceur(prog, GL_VERTEX_SHADER, &chainesSommets);
            delete [] chainesSommets;
        }

        const GLchar *chainesTessCtrl = ProgNuanceur::lireNuanceur( "./nuanceurs/tess.ctrl.glsl" );		
        if ( chainesTessCtrl != NULL )
        {
			chargerNuanceur(prog, GL_TESS_CONTROL_SHADER, &chainesTessCtrl);
            delete [] chainesTessCtrl;
        }

        const GLchar *chainesTessEval = ProgNuanceur::lireNuanceur( "./nuanceurs/tess.eval.glsl" );		
        if ( chainesTessEval != NULL )
        {
			chargerNuanceur(prog, GL_TESS_EVALUATION_SHADER, &chainesTessEval);
            delete [] chainesTessEval;
        }

        // attacher le nuanceur de fragments
        const GLchar *chainesFragments = ProgNuanceur::lireNuanceur( "./nuanceurs/tess.fragments.glsl" );
        if ( chainesFragments != NULL )
        {
			chargerNuanceur(prog, GL_FRAGMENT_SHADER, &chainesFragments);
            delete [] chainesFragments;
        }

        // faire l'édition des liens du programme
        glLinkProgram( prog );
        if ( !ProgNuanceur::afficherLogLink( prog ) ) exit(1);
    }
    else if (varsUnif.typeIllumination == 0) // Gouraud
    {
        // créer le programme
        prog = glCreateProgram();

        // attacher le nuanceur de sommets
        const GLchar *chainesSommets = ProgNuanceur::lireNuanceur( "./nuanceurs/gouraud.sommets.glsl" );		
        if ( chainesSommets != NULL )
        {
			chargerNuanceur(prog, GL_VERTEX_SHADER, &chainesSommets);
            delete [] chainesSommets;
        }

        // attacher le nuanceur de fragments
        const GLchar *chainesFragments = ProgNuanceur::lireNuanceur( "./nuanceurs/gouraud.fragments.glsl" );
        if ( chainesFragments != NULL )
        {
			chargerNuanceur(prog, GL_FRAGMENT_SHADER, &chainesFragments);
            delete [] chainesFragments;
        }

        // faire l'édition des liens du programme
        glLinkProgram( prog );
        if ( !ProgNuanceur::afficherLogLink( prog ) ) exit(1);
    }
    else if (varsUnif.typeIllumination == 1) // Phong
    {
        prog = glCreateProgram();

        // attacher le nuanceur de sommets
        const GLchar *chainesSommets = ProgNuanceur::lireNuanceur( "./nuanceurs/phong.sommets.glsl" );		
        if ( chainesSommets != NULL )
        {
			chargerNuanceur(prog, GL_VERTEX_SHADER, &chainesSommets);
            delete [] chainesSommets;
        }

        // attacher le nuanceur de fragments
        const GLchar *chainesFragments = ProgNuanceur::lireNuanceur( "./nuanceurs/phong.fragments.glsl" );
        if ( chainesFragments != NULL )
        {
			chargerNuanceur(prog, GL_FRAGMENT_SHADER, &chainesFragments);
            delete [] chainesFragments;
        }

        // faire l'édition des liens du programme
        glLinkProgram( prog );
        if ( !ProgNuanceur::afficherLogLink( prog ) ) exit(1);
    }

    // partie 3a:
    if ( Etat::utiliseTess ) 
    {
        if ( ( locVertex = glGetAttribLocation( prog, "Vertex" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de Vertex" << std::endl;
        if ( ( locTexCoord = glGetAttribLocation( prog, "TexCoord" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de TexCoord (partie 2)" << std::endl;
        if ( ( locTessLevelInner = glGetUniformLocation( prog, "TessLevelInner" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de TessLevelInner (partie 3a)" << std::endl;
        if ( ( locTessLevelOuter = glGetUniformLocation( prog, "TessLevelOuter" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de TessLevelOuter (partie 3a)" << std::endl;
        if ( ( locmatrModel = glGetUniformLocation( prog, "matrModel" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de matrModel" << std::endl;
        if ( ( locmatrVisu = glGetUniformLocation( prog, "matrVisu" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de matrVisu" << std::endl;
        if ( ( locmatrProj = glGetUniformLocation( prog, "matrProj" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de matrProj" << std::endl;
    }
    else
    {
        // demander la "Location" des variables
        if ( ( locVertex = glGetAttribLocation( prog, "Vertex" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de Vertex" << std::endl;
        if ( ( locNormal = glGetAttribLocation( prog, "Normal" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de Normal (partie 1)" << std::endl;
        if ( ( locTexCoord = glGetAttribLocation( prog, "TexCoord" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de TexCoord (partie 2)" << std::endl;
        if ( ( locmatrModel = glGetUniformLocation( prog, "matrModel" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de matrModel" << std::endl;
        if ( ( locmatrVisu = glGetUniformLocation( prog, "matrVisu" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de matrVisu" << std::endl;
        if ( ( locmatrProj = glGetUniformLocation( prog, "matrProj" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de matrProj" << std::endl;
        if ( ( locmatrNormale = glGetUniformLocation( prog, "matrNormale" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de matrNormale (partie 1)" << std::endl;
        if ( ( loclaTextureCoul = glGetUniformLocation( prog, "laTextureCoul" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de laTextureCoul (partie 2)" << std::endl;
        if ( ( loclaTextureNorm = glGetUniformLocation( prog, "laTextureNorm" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de laTextureNorm (partie 3b)" << std::endl;

        if ( ( indLightSource = glGetUniformBlockIndex( prog, "LightSourceParameters" ) ) == GL_INVALID_INDEX ) std::cerr << "!!! pas trouvé l'\"index\" de LightSource" << std::endl;
        if ( ( indFrontMaterial = glGetUniformBlockIndex( prog, "MaterialParameters" ) ) == GL_INVALID_INDEX ) std::cerr << "!!! pas trouvé l'\"index\" de FrontMaterial" << std::endl;
        if ( ( indLightModel = glGetUniformBlockIndex( prog, "LightModelParameters" ) ) == GL_INVALID_INDEX ) std::cerr << "!!! pas trouvé l'\"index\" de LightModel" << std::endl;
        if ( ( indvarsUnif = glGetUniformBlockIndex( prog, "varsUnif" ) ) == GL_INVALID_INDEX ) std::cerr << "!!! pas trouvé l'\"index\" de varsUnif" << std::endl;

        // charger les blocs de variables uniformes (ubo)
        {
            glBindBuffer( GL_UNIFORM_BUFFER, ubo[0] );
            glBufferData( GL_UNIFORM_BUFFER, sizeof(LightSource), &LightSource, GL_DYNAMIC_COPY );
            glBindBuffer( GL_UNIFORM_BUFFER, 0 );
            const GLuint bindingIndex = 0;
            glBindBufferBase( GL_UNIFORM_BUFFER, bindingIndex, ubo[0] );
            glUniformBlockBinding( prog, indLightSource, bindingIndex );
        }
        {
            glBindBuffer( GL_UNIFORM_BUFFER, ubo[1] );
            glBufferData( GL_UNIFORM_BUFFER, sizeof(FrontMaterial), &FrontMaterial, GL_DYNAMIC_COPY );
            glBindBuffer( GL_UNIFORM_BUFFER, 0 );
            const GLuint bindingIndex = 1;
            glBindBufferBase( GL_UNIFORM_BUFFER, bindingIndex, ubo[1] );
            glUniformBlockBinding( prog, indFrontMaterial, bindingIndex );
        }
        {
            glBindBuffer( GL_UNIFORM_BUFFER, ubo[2] );
            glBufferData( GL_UNIFORM_BUFFER, sizeof(LightModel), &LightModel, GL_DYNAMIC_COPY );
            glBindBuffer( GL_UNIFORM_BUFFER, 0 );
            const GLuint bindingIndex = 2;
            glBindBufferBase( GL_UNIFORM_BUFFER, bindingIndex, ubo[2] );
            glUniformBlockBinding( prog, indLightModel, bindingIndex );
        }
        {
            glBindBuffer( GL_UNIFORM_BUFFER, ubo[3] );
            glBufferData( GL_UNIFORM_BUFFER, sizeof(varsUnif), &varsUnif, GL_DYNAMIC_COPY );
            glBindBuffer( GL_UNIFORM_BUFFER, 0 );
            const GLuint bindingIndex = 3;
            glBindBufferBase( GL_UNIFORM_BUFFER, bindingIndex, ubo[3] );
            glUniformBlockBinding( prog, indvarsUnif, bindingIndex );
        }
        glBindBuffer( GL_UNIFORM_BUFFER, 0 );
    }
}

// initialisation d'openGL
void FenetreTP::initialiser()
{
    // couleur de l'arrière-plan
    glm::vec4 couleurFond( 0.2, 0.2, 0.2, 1.0 );
    glClearColor( couleurFond.r, couleurFond.g, couleurFond.b, couleurFond.a );

    // activer les etats openGL
    glEnable( GL_DEPTH_TEST );

    // charger les textures
    chargerTextures();

    // allouer les UBO pour les variables uniformes
    glGenBuffers( 4, ubo );

    // charger les nuanceurs
    chargerProgNuanceurs();
    glUseProgram( prog );

    // partie 1: créer le cube
    /*         +Y                    */
    /*   3+-----------+2             */
    /*    |\          |\             */
    /*    | \         | \            */
    /*    |  \        |  \           */
    /*    |  7+-----------+6         */
    /*    |   |       |   |          */
    /*    |   |       |   |          */
    /*   0+---|-------+1  |          */
    /*     \  |        \  |     +X   */
    /*      \ |         \ |          */
    /*       \|          \|          */
    /*       4+-----------+5         */
    /*             +Z                */

    const GLfloat sommets[3*4*6] =
    {
       -0.9,  0.9,  0.9,   0.9,  0.9,  0.9,  -0.9,  0.9, -0.9,   0.9,  0.9, -0.9,  // P7,P6,P3,P2 +Y
        0.9, -0.9, -0.9,  -0.9, -0.9, -0.9,   0.9,  0.9, -0.9,  -0.9,  0.9, -0.9,  // P1,P0,P2,P3 -Z
        0.9, -0.9,  0.9,   0.9, -0.9, -0.9,   0.9,  0.9,  0.9,   0.9,  0.9, -0.9,  // P5,P1,P6,P2 +X
       -0.9, -0.9,  0.9,   0.9, -0.9,  0.9,  -0.9,  0.9,  0.9,   0.9,  0.9,  0.9,  // P4,P5,P7,P6 +Z
       -0.9, -0.9, -0.9,  -0.9, -0.9,  0.9,  -0.9,  0.9, -0.9,  -0.9,  0.9,  0.9,  // P0,P4,P3,P7 -X
        0.9, -0.9,  0.9,  -0.9, -0.9,  0.9,   0.9, -0.9, -0.9,  -0.9, -0.9, -0.9,  // P5,P4,P1,P0 -Y
    };

    // partie 1: définir les normales
    GLfloat normales[3 * 4 * 6] = { 0, +1, 0, 0, +1, 0, 0, +1, 0, 0, +1, 0,
                                    0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1,
                                    +1, 0, 0, +1, 0, 0, +1, 0, 0, +1, 0, 0,
                                    0, 0, +1, 0, 0, +1, 0, 0, +1, 0, 0, +1,
                                    -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0,
                                    0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0
    };

    // partie 2: définir les coordonnées de texture
     const GLfloat             // les différentes parties du monde  (voir figure 15)
     MOx1=0.0,   MOy1=0.0,  MOx2=3.0,  MOy2=3.0,    // le Monde
     ASx1=0.2,   ASy1=0.2,  ASx2=0.45, ASy2=0.6,    // l'Amérique du Sud
     AMx1=0.45,  AMy1=0.3,  AMx2=0.65, AMy2=0.7,    // l'Afrique + Moyen-Orient
     QCx1=0.275, QCy1=0.75, QCx2=0.35, QCy2=0.85,   // le Québec
     EUx1=0.45,  EUy1=0.7,  EUx2=0.55, EUy2=0.825,  // l'Europe (avec l'Angleterre ;))
     AUx1=0.8,   AUy1=0.25, AUx2=0.95, AUy2=0.45;   // l'Australie

     GLfloat texcoordsTerre[2*4*6] = {
         MOx1,   MOy1,  MOx2,  MOy1, MOx1, MOy2, MOx2, MOy2,      // le Monde
         ASx1,   ASy1,  ASx2,  ASy1, ASx1, ASy2, ASx2, ASy2,      // l'Amérique du Sud
         AMx1,   AMy1,  AMx2,  AMy1, AMx1, AMy2, AMx2, AMy2,      // l'Afrique + Moyen-Orient
         QCx1,   QCy1,  QCx2,  QCy1, QCx1, QCy2, QCx2, QCy2,      // le Québec
         EUx1,   EUy1,  EUx2,  EUy1, EUx1, EUy2, EUx2, EUy2,      // l'Europe (avec l'Angleterre ;))
         AUx1,   AUy1,  AUx2,  AUy1, AUx1, AUy2, AUx2, AUy2,      // l'Australie
     };  // les coordonnées de texture pour la Terre (voir figure 15)

     GLfloat texcoordsAutre[2 * 4 * 6] = {
        0, 0, 1, 0, 0, 1, 1, 1,
        0, 0, 1, 0, 0, 1, 1, 1,
        0, 0, 1, 0, 0, 1, 1, 1,
        0, 0, 1, 0, 0, 1, 1, 1,
        0, 0, 1, 0, 0, 1, 1, 1,
        0, 0, 1, 0, 0, 1, 1, 1,
     };  // (0,0), (+1,0), etc.


    // allouer les objets OpenGL
    glGenVertexArrays( 3, vao );
    glGenBuffers( 6, vbo );
    // initialiser le VAO
    glBindVertexArray( vao[0] );

    // charger le VBO pour les sommets
    glBindBuffer( GL_ARRAY_BUFFER, vbo[0] );
    glBufferData( GL_ARRAY_BUFFER, sizeof(sommets), sommets, GL_STATIC_DRAW );
    glVertexAttribPointer( locVertex, 3, GL_FLOAT, GL_FALSE, 0, 0 );
    glEnableVertexAttribArray(locVertex);
    // partie 1: charger le VBO pour les normales
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData( GL_ARRAY_BUFFER, sizeof(normales), normales, GL_STATIC_DRAW);
    glVertexAttribPointer(locNormal, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(locNormal);
    // partie 2: charger les deux VBO pour les coordonnées de texture: celle pour la Terre sur le cube et pour les autres textures
    glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(texcoordsTerre), texcoordsTerre, GL_STATIC_DRAW);
    //glVertexAttribPointer(locTexCoord, 2, GL_FLOAT, GL_FALSE, 0, 0);
    //glEnableVertexAttribArray(locTexCoord);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(texcoordsAutre), texcoordsAutre, GL_STATIC_DRAW);
    //glVertexAttribPointer(locTexCoord, 2, GL_FLOAT, GL_FALSE, 0, 0);
    //glEnableVertexAttribArray(locTexCoord);

    glBindVertexArray(0);

    // initialiser le VAO pour le terrain
    glBindVertexArray( vao[2] );
    glBindBuffer( GL_ARRAY_BUFFER, vbo[5] );
    GLfloat sol[] = {
        -10.0, 0.0, -10.0, 0.0, 0.0,
        -10.0, 0.0,  10.0, 0.0, 1.0,
         10.0, 0.0,  10.0, 1.0, 1.0,
         10.0, 0.0, -10.0, 1.0, 0.0
    };
    glBufferData( GL_ARRAY_BUFFER, sizeof(sol), sol, GL_STATIC_DRAW );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), 0 );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer( 8, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*) (3 * sizeof(GLfloat)) );
    glEnableVertexAttribArray(8);

    glBindVertexArray( 0 );
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // initialiser le VAO pour une ligne (montrant la direction de la lumiére)
    glGenBuffers( 1, &vboLumi );
    glBindVertexArray( vao[1] );
    GLfloat coords[] = { 0., 0., 0., 0., 0., 1. };
    glBindBuffer( GL_ARRAY_BUFFER, vboLumi );
    glBufferData( GL_ARRAY_BUFFER, sizeof(coords), coords, GL_STATIC_DRAW );
    glVertexAttribPointer( locVertexBase, 3, GL_FLOAT, GL_FALSE, 0, 0 );
    glEnableVertexAttribArray(locVertexBase);
    glBindVertexArray(0);

    // créer quelques autres formes
    sphere = new FormeSphere( 1.0, 32, 32 );
    sphereLumi = new FormeSphere( 0.3, 10, 10 );
    theiere = new FormeTheiere( );
    tore = new FormeTore( 0.4, 0.8, 32, 32 );
    cylindre = new FormeCylindre( 0.3, 0.3, 3.0, 32, 32 );

    if ( getenv("DEMO") != NULL )
    {
        Etat::enmouvement = true;
        Etat::englissement = true;
    }
}

void FenetreTP::conclure()
{
    glUseProgram( 0 );
    glDeleteVertexArrays( 2, vao );
    glDeleteBuffers( 4, vbo );
    glDeleteBuffers( 1, &vboLumi );
    glDeleteBuffers( 4, ubo );
    delete sphere;
    delete sphereLumi;
    delete theiere;
    delete tore;
    delete cylindre;
}

void afficherModele()
{
    // partie 2: paramètres de texture
    glActiveTexture( GL_TEXTURE0 ); // l'unité de texture 0
    if ( varsUnif.iTexCoul )
        glBindTexture( GL_TEXTURE_2D, texturesCoul[varsUnif.iTexCoul-1] );
    else
        glBindTexture( GL_TEXTURE_2D, 0 );

    // Changement du vbo en cas de cube avec planete partie 2:
    glBindVertexArray(vao[0]);
    if( varsUnif.iTexCoul == 1 )
        glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
    else
        glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
    glVertexAttribPointer(locTexCoord, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(locTexCoord);


    glActiveTexture( GL_TEXTURE1 ); // l'unité de texture 1
    if ( varsUnif.iTexNorm )
        glBindTexture( GL_TEXTURE_2D, texturesNorm[varsUnif.iTexNorm-1] );
    else
        glBindTexture( GL_TEXTURE_2D, 0 );

    // Dessiner le modèle
    matrModel.PushMatrix(); {

        // mise à l'échelle
        matrModel.Scale( 5.0, 5.0, 5.0 );

        glPatchParameteri( GL_PATCH_VERTICES, 4 );

        // afficher le terrain
        if ( Etat::utiliseTess )
        {
            // partie 3a: afficher le terrain avec des GL_PATCHES
            glBindVertexArray(vao[2]);
            glActiveTexture(GL_TEXTURE0); // l'unité de texture 0
            
            glBindBuffer(GL_ARRAY_BUFFER, vbo[5]);
            glBindTexture(GL_TEXTURE_2D, heightMapTex);

            glVertexAttribPointer(locTexCoord, 2, GL_FLOAT, GL_FALSE, 0, 0);

            glDrawArrays(GL_PATCHES, 0, 4);
        }   
        else
        {
            glUniformMatrix3fv(locmatrNormale, 1, GL_TRUE, glm::value_ptr(glm::inverse(glm::mat3(matrVisu.getMatr() * matrModel.getMatr()))));
            switch ( Etat::modele )
            {
            default:
            case 1:
                // afficher le cube
                glUniformMatrix4fv( locmatrModel, 1, GL_FALSE, matrModel );
                
                // (partie 1: ne pas oublier de calculer et donner une matrice pour les transformations des normales)
                //glm::mat3 matrVM = glm::mat3(matrVisu.getMatr() * matrModel.getMatr());
                //glm::mat3 matrNormale = glm::inverse(matrVM);
                //glUniformMatrix3fv(locmatrNormale, 1, GL_TRUE, // on transpose
                //    glm::value_ptr(matrNormale));
                
                glBindVertexArray( vao[0] );
                glDrawArrays( GL_TRIANGLE_STRIP,  0, 4 );
                glDrawArrays( GL_TRIANGLE_STRIP,  4, 4 );
                glDrawArrays( GL_TRIANGLE_STRIP,  8, 4 );
                glDrawArrays( GL_TRIANGLE_STRIP, 12, 4 );
                glDrawArrays( GL_TRIANGLE_STRIP, 16, 4 );
                glDrawArrays( GL_TRIANGLE_STRIP, 20, 4 );
                glBindVertexArray( 0 );
                break;
            case 2:
                matrModel.Scale( 0.25, 0.25, 0.25 );
                matrModel.Translate( 0.0, -1.5, 0.0 );
                glUniformMatrix4fv( locmatrModel, 1, GL_FALSE, matrModel );
                theiere->afficher();
                break;
            case 3:
                matrModel.Rotate( -90, 1.0, 0.0, 0.0 );
                glUniformMatrix4fv( locmatrModel, 1, GL_FALSE, matrModel );
                sphere->afficher();
                break;
            case 4:
                glUniformMatrix4fv( locmatrModel, 1, GL_FALSE, matrModel );
                tore->afficher();
                break;
            case 5:
                matrModel.Translate( 0.0, 0.0, -1.5 );
                glUniformMatrix4fv( locmatrModel, 1, GL_FALSE, matrModel );
                cylindre->afficher();
                break;
            }
        }
    } matrModel.PopMatrix(); glUniformMatrix4fv( locmatrModel, 1, GL_FALSE, matrModel );
}

void afficherLumieres()
{
    // Dessiner les lumières
    for ( int i = 0 ; i < 3 ; ++i )
    {
        glVertexAttrib3f( locColorBase, 2*LightSource.diffuse[i].r, 2*LightSource.diffuse[i].g, 2*LightSource.diffuse[i].b ); // couleur
#if 1
        // dessiner une ligne vers le spot
        GLfloat coords[] =
        {
            LightSource.position[i].x                               , LightSource.position[i].y                               , LightSource.position[i].z,
            LightSource.position[i].x+LightSource.spotDirection[i].x, LightSource.position[i].y+LightSource.spotDirection[i].y, LightSource.position[i].z+LightSource.spotDirection[i].z
        };
        glLineWidth( 3.0 );
        glBindVertexArray( vao[1] );
        matrModel.PushMatrix(); {
            glBindBuffer( GL_ARRAY_BUFFER, vboLumi );
            glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(coords), coords );
            glDrawArrays( GL_LINES, 0, 2 );
        } matrModel.PopMatrix(); glUniformMatrix4fv( locmatrModelBase, 1, GL_FALSE, matrModel );
        glBindVertexArray( 0 );
        glLineWidth( 1.0 );
#endif

        // dessiner une sphère à la position de la lumière
        matrModel.PushMatrix(); {
            matrModel.Translate( LightSource.position[i].x, LightSource.position[i].y, LightSource.position[i].z );
            glUniformMatrix4fv( locmatrModelBase, 1, GL_FALSE, matrModel );
            sphereLumi->afficher();
        } matrModel.PopMatrix(); glUniformMatrix4fv( locmatrModelBase, 1, GL_FALSE, matrModel );
    }
}

// fonction d'affichage
void FenetreTP::afficherScene()
{
    // effacer l'ecran et le tampon de profondeur
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glUseProgram( progBase );

    // définir le pipeline graphique
    if ( Etat::enPerspective )
    {
        matrProj.Perspective( 35.0, (GLdouble)largeur_ / (GLdouble)hauteur_,
                              0.1, 60.0 );
    }
    else
    {
        const GLfloat d = 8.0;
        if ( largeur_ <= hauteur_ )
        {
            matrProj.Ortho( -d, d,
                            -d*(GLdouble)hauteur_ / (GLdouble)largeur_,
                            d*(GLdouble)hauteur_ / (GLdouble)largeur_,
                            0.1, 60.0 );
        }
        else
        {
            matrProj.Ortho( -d*(GLdouble)largeur_ / (GLdouble)hauteur_,
                            d*(GLdouble)largeur_ / (GLdouble)hauteur_,
                            -d, d,
                            0.1, 60.0 );
        }
    }
    glUniformMatrix4fv( locmatrProjBase, 1, GL_FALSE, matrProj );

    camera.definir();
    glUniformMatrix4fv( locmatrVisuBase, 1, GL_FALSE, matrVisu );

    matrModel.LoadIdentity();
    glUniformMatrix4fv( locmatrModelBase, 1, GL_FALSE, matrModel );

    // afficher les axes
    if ( Etat::afficheAxes ) FenetreTP::afficherAxes( 8.0 );

    // dessiner la scène
    afficherLumieres();

    glUseProgram( prog );

    // mettre à jour les blocs de variables uniformes (ubo)
    {
        glBindBuffer( GL_UNIFORM_BUFFER, ubo[0] );
        GLvoid *p = glMapBuffer( GL_UNIFORM_BUFFER, GL_WRITE_ONLY );
        memcpy( p, &LightSource, sizeof(LightSource) );
        glUnmapBuffer( GL_UNIFORM_BUFFER );
    }
    {
        glBindBuffer( GL_UNIFORM_BUFFER, ubo[1] );
        GLvoid *p = glMapBuffer( GL_UNIFORM_BUFFER, GL_WRITE_ONLY );
        memcpy( p, &FrontMaterial, sizeof(FrontMaterial) );
        glUnmapBuffer( GL_UNIFORM_BUFFER );
    }
    {
        glBindBuffer( GL_UNIFORM_BUFFER, ubo[2] );
        GLvoid *p = glMapBuffer( GL_UNIFORM_BUFFER, GL_WRITE_ONLY );
        memcpy( p, &LightModel, sizeof(LightModel) );
        glUnmapBuffer( GL_UNIFORM_BUFFER );
    }
    {
        glBindBuffer( GL_UNIFORM_BUFFER, ubo[3] );
        GLvoid *p = glMapBuffer( GL_UNIFORM_BUFFER, GL_WRITE_ONLY );
        memcpy( p, &varsUnif, sizeof(varsUnif) );
        glUnmapBuffer( GL_UNIFORM_BUFFER );
    }
    glBindBuffer( GL_UNIFORM_BUFFER, 0 );

    // mettre à jour les matrices et autres uniformes
    glUniformMatrix4fv( locmatrProj, 1, GL_FALSE, matrProj );
    glUniformMatrix4fv( locmatrVisu, 1, GL_FALSE, matrVisu );
    glUniformMatrix4fv( locmatrModel, 1, GL_FALSE, matrModel );

    if ( Etat::utiliseTess )
    {
        glUniform1f( locTessLevelInner, Etat::TessLevelInner );
        glUniform1f( locTessLevelOuter, Etat::TessLevelOuter );
    }
    else
    {
        glUniform1i( loclaTextureCoul, 0 ); // '0' => utilisation de GL_TEXTURE0
        glUniform1i( loclaTextureNorm, 1 ); // '1' => utilisation de GL_TEXTURE1
    }

    afficherModele();

    // permuter tampons avant et arrière
    swap();
}

// fonction de redimensionnement de la fenêtre graphique
void FenetreTP::redimensionner( GLsizei w, GLsizei h )
{
    glViewport( 0, 0, w, h );
}

static void echoEtatsIllum( )
{
    static std::string illuminationStr[] = { "0:Gouraud", "1:Phong  " };
    static std::string reflexionStr[] = { "0:Phong", "1:Blinn" };
    static std::string spotStr[] = { "0:OpenGL", "1:Direct3D" };
    std::cout << " {p} modèle d'illumination = " << illuminationStr[varsUnif.typeIllumination]
              << "   {r} refléxion spéculaire = " << reflexionStr[varsUnif.utiliseBlinn]
              << "   {d} spot = " << spotStr[varsUnif.utiliseDirect]
              << std::endl;
}

static void echoEtatsTexture( )
{
    static std::string fonce[] = { "0:normalement", "1:coloré", "2:transparent" };
    std::cout << " {t} varsUnif.iTexCoul = " << varsUnif.iTexCoul
              << "   {e} varsUnif.iTexNorm = " << varsUnif.iTexNorm
              << std::endl;
}

// fonction de gestion du clavier
void FenetreTP::clavier( TP_touche touche )
{
    // traitement des touches q et echap
    switch ( touche )
    {
    case TP_ECHAP:
    case TP_q: // Quitter l'application
        quit();
        break;

    case TP_x: // Activer/désactiver l'affichage des axes
        Etat::afficheAxes = !Etat::afficheAxes;
        std::cout << "// Affichage des axes ? " << ( Etat::afficheAxes ? "OUI" : "NON" ) << std::endl;
        break;

    case TP_9: // Permuter l'utilisation des nuanceurs de tessellation
        Etat::utiliseTess = !Etat::utiliseTess;
        std::cout << " Etat::utiliseTess=" << Etat::utiliseTess << std::endl;
        // lorsqu'avec la tessellation, forcer le modèle cube
        if ( Etat::utiliseTess ) { Etat::modele = 1; std::cout << " (Avec tessellation, on n'utilise que le cube.)" << std::endl; }
        // recréer les nuanceurs
        chargerProgNuanceurs();
        break;

    case TP_v: // Recharger les fichiers des nuanceurs et recréer le programme
        chargerProgNuanceurs();
        std::cout << "// Recharger nuanceurs" << std::endl;
        break;

    case TP_p: // Alterner entre le modèle d'illumination: Gouraud, Phong
        if ( ++varsUnif.typeIllumination > 1 ) varsUnif.typeIllumination = 0;
        chargerProgNuanceurs();
        echoEtatsIllum( );
        break;

    case TP_r: // Alterner entre le modèle de réflexion spéculaire: Phong, Blinn
        varsUnif.utiliseBlinn = !varsUnif.utiliseBlinn;
        echoEtatsIllum( );
        break;

    case TP_d: // Alterner entre le modèle de spot: OpenGL, Direct3D
        varsUnif.utiliseDirect = !varsUnif.utiliseDirect;
        echoEtatsIllum( );
        break;

    case TP_h: // Incrémenter le coefficient de brillance
    case TP_CROCHETDROIT:
        FrontMaterial.shininess *= 1.1;
        std::cout << " FrontMaterial.shininess=" << FrontMaterial.shininess << std::endl;
        break;
    case TP_b: // Décrémenter le coefficient de brillance
    case TP_CROCHETGAUCHE:
        FrontMaterial.shininess /= 1.1; if ( FrontMaterial.shininess < 0.0 ) FrontMaterial.shininess = 0.0;
        std::cout << " FrontMaterial.shininess=" << FrontMaterial.shininess << std::endl;
        break;

    case TP_c: // Incrémenter l'angle d'ouverture du cône du spot
    case TP_EGAL:
    case TP_PLUS:
        LightSource.spotAngleOuverture += 2.0;
        if ( LightSource.spotAngleOuverture > 90.0 ) LightSource.spotAngleOuverture = 90.0;
        std::cout <<  " spotAngleOuverture=" << LightSource.spotAngleOuverture << std::endl;
        break;
    case TP_f: // Décrémenter l'angle d'ouverture du cône du spot
    case TP_MOINS:
    case TP_SOULIGNE:
        LightSource.spotAngleOuverture -= 2.0;
        if ( LightSource.spotAngleOuverture < 0.0 ) LightSource.spotAngleOuverture = 0.0;
        std::cout <<  " spotAngleOuverture=" << LightSource.spotAngleOuverture << std::endl;
        break;

    case TP_w: // Incrémenter l'exposant du spot
        LightSource.spotExposant += 0.3;
        if ( LightSource.spotExposant > 89.0 ) LightSource.spotExposant = 89.0;
        std::cout <<  " spotExposant=" << LightSource.spotExposant << std::endl;
        break;
    case TP_s: // Décrémenter l'exposant du spot
        LightSource.spotExposant -= 0.3;
        if ( LightSource.spotExposant < 0.0 ) LightSource.spotExposant = 0.0;
        std::cout <<  " spotExposant=" << LightSource.spotExposant << std::endl;
        break;

    case TP_m: // Choisir le modèle affiché: cube, tore, sphère, théière, cylindre, cône
        if ( ++Etat::modele > 5 ) Etat::modele = 1;
        // lorsqu'avec la tessellation, forcer le modèle cube
        if ( Etat::utiliseTess ) { Etat::modele = 1; std::cout << " (Avec tessellation, on n'utilise que le cube.)" << std::endl; }
        std::cout << " Etat::modele=" << Etat::modele << std::endl;
        break;

    case TP_t: // Choisir la texture de couleurs utilisée: aucune, Terre, échiquier, mur, métal, mosaique
        varsUnif.iTexCoul++;
        if ( varsUnif.iTexCoul > texCoulNum ) varsUnif.iTexCoul = 0;
        echoEtatsTexture( );
        break;

    case TP_e: // Choisir la texture de normales utilisée: aucune, pierre, bulles, mur, brique, circuit
        varsUnif.iTexNorm++;
        if ( varsUnif.iTexNorm > texNormNum ) varsUnif.iTexNorm = 0;
        echoEtatsTexture( );
        break;

    case TP_i: // Augmenter le niveau de tessellation interne
        ++Etat::TessLevelInner;
        std::cout << " TessLevelInner=" << Etat::TessLevelInner << " TessLevelOuter=" << Etat::TessLevelOuter << std::endl;
        glPatchParameteri( GL_PATCH_DEFAULT_INNER_LEVEL, Etat::TessLevelInner );
        break;
    case TP_k: // Diminuer le niveau de tessellation interne
        if ( --Etat::TessLevelInner < 1 ) Etat::TessLevelInner = 1;
        std::cout << " TessLevelInner=" << Etat::TessLevelInner << " TessLevelOuter=" << Etat::TessLevelOuter << std::endl;
        glPatchParameteri( GL_PATCH_DEFAULT_INNER_LEVEL, Etat::TessLevelInner );
        break;

    case TP_o: // Augmenter le niveau de tessellation externe
        ++Etat::TessLevelOuter;
        std::cout << " TessLevelInner=" << Etat::TessLevelInner << " TessLevelOuter=" << Etat::TessLevelOuter << std::endl;
        glPatchParameteri( GL_PATCH_DEFAULT_OUTER_LEVEL, Etat::TessLevelOuter );
        break;
    case TP_l: // Diminuer le niveau de tessellation externe
        if ( --Etat::TessLevelOuter < 1 ) Etat::TessLevelOuter = 1;
        std::cout << " TessLevelInner=" << Etat::TessLevelInner << " TessLevelOuter=" << Etat::TessLevelOuter << std::endl;
        glPatchParameteri( GL_PATCH_DEFAULT_OUTER_LEVEL, Etat::TessLevelOuter );
        break;

    case TP_u: // Augmenter les deux niveaux de tessellation
        ++Etat::TessLevelOuter;
        Etat::TessLevelInner = Etat::TessLevelOuter;
        std::cout << " TessLevelInner=" << Etat::TessLevelInner << " TessLevelOuter=" << Etat::TessLevelOuter << std::endl;
        glPatchParameteri( GL_PATCH_DEFAULT_OUTER_LEVEL, Etat::TessLevelOuter );
        glPatchParameteri( GL_PATCH_DEFAULT_INNER_LEVEL, Etat::TessLevelInner );
        break;
    case TP_j: // Diminuer les deux niveaux de tessellation
        if ( --Etat::TessLevelOuter < 1 ) Etat::TessLevelOuter = 1;
        Etat::TessLevelInner = Etat::TessLevelOuter;
        std::cout << " TessLevelInner=" << Etat::TessLevelInner << " TessLevelOuter=" << Etat::TessLevelOuter << std::endl;
        glPatchParameteri( GL_PATCH_DEFAULT_OUTER_LEVEL, Etat::TessLevelOuter );
        glPatchParameteri( GL_PATCH_DEFAULT_INNER_LEVEL, Etat::TessLevelInner );
        break;

    case TP_1:
        Etat::modele = 3;
        varsUnif.iTexCoul = 0;
        varsUnif.iTexNorm = 0;
        break;
    case TP_2:
        Etat::modele = 3;
        varsUnif.iTexCoul = 0;
        varsUnif.iTexNorm = 5;
        break;
    case TP_3:
        Etat::modele = 4;
        varsUnif.iTexCoul = 2;
        varsUnif.iTexNorm = 4;
        break;
    case TP_4:
        Etat::modele = 2;
        varsUnif.iTexCoul = 1;
        varsUnif.iTexNorm = 1;
        break;
    case TP_5:
        Etat::modele = 2;
        varsUnif.iTexCoul = 5;
        varsUnif.iTexNorm = 5;
        break;
    case TP_6:
        varsUnif.iTexCoul = 0;
        varsUnif.iTexNorm = 5;
        break;
    case TP_7:
        varsUnif.iTexCoul = 2;
        varsUnif.iTexNorm = 0;
        break;
    case TP_8:
        varsUnif.iTexCoul = 2;
        varsUnif.iTexNorm = 2;
        break;

    // case TP_POINT: // Augmenter l'effet du déplacement (voir Apprentissage supplémentaire)
    //     Etat::facteurDeform += 0.01;
    //     std::cout << " facteurDeform=" << Etat::facteurDeform << std::endl;
    //     break;
    // case TP_VIRGULE: // Diminuer l'effet du déplacement (voir Apprentissage supplémentaire)
    //     Etat::facteurDeform -= 0.01;
    //     std::cout << " facteurDeform=" << Etat::facteurDeform << std::endl;
    //     break;

    case TP_BARREOBLIQUE: // Permuter la projection: perspective ou orthogonale
        Etat::enPerspective = !Etat::enPerspective;
        break;

    case TP_g: // Permuter l'affichage en fil de fer ou plein
        Etat::modePolygone = ( Etat::modePolygone == GL_FILL ) ? GL_LINE : GL_FILL;
        glPolygonMode( GL_FRONT_AND_BACK, Etat::modePolygone );
        break;

    case TP_n: // Utiliser ou non les normales calculées comme couleur (pour le débogage)
        varsUnif.afficheNormales = !varsUnif.afficheNormales;
        break;

    case TP_0: // Replacer Caméra et Lumière afin d'avoir une belle vue
        camera.theta = -7.0; camera.phi = -5.0; camera.dist = 30.0;
        LightSource.position[0] = posLumiInit[0];
        LightSource.position[1] = posLumiInit[1];
        LightSource.position[2] = posLumiInit[2];
        LightSource.spotDirection[0] = spotDirInit[0];
        LightSource.spotDirection[1] = spotDirInit[1];
        LightSource.spotDirection[2] = spotDirInit[2];
        break;

    case TP_z: // Remettre le temps à 0
        Etat::temps = 0;
        varsUnif.tempsGlissement = 0;
        break;

    case TP_a: // Le temps avance ou est à l'arrêt
        Etat::englissement = !Etat::englissement;
        break;

    case TP_y:
        varsUnif.utiliseSpot = !varsUnif.utiliseSpot;
        break;

    case TP_ESPACE: // Permuter la rotation automatique du modèle
        Etat::enmouvement = !Etat::enmouvement;
        break;

    default:
        std::cout << " touche inconnue : " << (char) touche << std::endl;
        imprimerFichier( "touches.txt" );
        break;
    }

}

// fonction callback pour un clic de souris
// la dernière position de la souris
static enum { deplaceCam, deplaceSpotDirection, deplaceLumiPosition } deplace = deplaceCam;
static bool pressed = false;
void FenetreTP::sourisClic( int button, int state, int x, int y )
{
    pressed = ( state == TP_PRESSE );
    if ( pressed )
    {
        // on vient de presser la souris
        Etat::sourisPosPrec.x = x;
        Etat::sourisPosPrec.y = y;
        switch ( button )
        {
        case TP_BOUTON_GAUCHE: // Tourner l'objet
            deplace = deplaceCam;
            break;
        case TP_BOUTON_MILIEU: // Modifier l'orientation du spot
            deplace = deplaceSpotDirection;
            break;
        case TP_BOUTON_DROIT: // Déplacer la lumière
            deplace = deplaceLumiPosition;
            break;
        }
        if ( deplace != deplaceCam )
        {
            glm::mat4 M = matrModel;
            glm::mat4 V = matrVisu;
            glm::mat4 P = matrProj;
            glm::vec4 cloture( 0, 0, largeur_, hauteur_ );
            glm::vec2 ecranLumi0 = glm::vec2( glm::project( glm::vec3(LightSource.position[0]), V*M, P, cloture ) );
            glm::vec2 ecranLumi1 = glm::vec2( glm::project( glm::vec3(LightSource.position[1]), V*M, P, cloture ) );
            glm::vec2 ecranLumi2 = glm::vec2( glm::project( glm::vec3(LightSource.position[2]), V*M, P, cloture ) );
            glm::vec2 ecranXY( x, hauteur_-y );
            if ( ( glm::distance( ecranLumi0, ecranXY ) < glm::distance( ecranLumi1, ecranXY ) ) &&
                 ( glm::distance( ecranLumi0, ecranXY ) < glm::distance( ecranLumi2, ecranXY ) ) )
                Etat::curLumi = 0;
            else if ( glm::distance( ecranLumi1, ecranXY ) < glm::distance( ecranLumi2, ecranXY ) )
                Etat::curLumi = 1;
            else
                Etat::curLumi = 2;
        }
    }
    else
    {
        // on vient de relâcher la souris
    }
}

void FenetreTP::sourisMolette( int x, int y ) // Changer la taille du spot
{
    const int sens = +1;
    LightSource.spotAngleOuverture += sens*y;
    if ( LightSource.spotAngleOuverture > 90.0 ) LightSource.spotAngleOuverture = 90.0;
    if ( LightSource.spotAngleOuverture < 0.0 ) LightSource.spotAngleOuverture = 0.0;
    std::cout <<  " spotAngleOuverture=" << LightSource.spotAngleOuverture << std::endl;
}

// fonction de mouvement de la souris
void FenetreTP::sourisMouvement( int x, int y )
{
    if ( pressed )
    {
        int dx = x - Etat::sourisPosPrec.x;
        int dy = y - Etat::sourisPosPrec.y;
        glm::mat4 M = matrModel;
        glm::mat4 V = matrVisu;
        glm::mat4 P = matrProj;
        glm::vec4 cloture( 0, 0, largeur_, hauteur_ );
        switch ( deplace )
        {
        case deplaceCam:
            camera.theta -= dx / 3.0;
            camera.phi   -= dy / 3.0;
            break;
        case deplaceSpotDirection:
            {
                //LightSource.spotDirection[Etat::curLumi].x += 0.06 * dx;
                //LightSource.spotDirection[Etat::curLumi].y -= 0.06 * dy;
                // obtenir les coordonnées d'écran correspondant à la pointe du vecteur
                glm::vec3 ecranLumi = glm::project( glm::vec3(LightSource.position[Etat::curLumi]+LightSource.spotDirection[Etat::curLumi]), V*M, P, cloture );
                // définir la nouvelle position (en utilisant la profondeur actuelle)
                glm::vec3 ecranPos( x, hauteur_-y, ecranLumi[2] );
                // placer la lumière à cette nouvelle position
                LightSource.spotDirection[Etat::curLumi] = glm::vec4( glm::unProject( ecranPos, V*M, P, cloture ), 1.0 ) - LightSource.position[Etat::curLumi];
                // normaliser sa longueur
                LightSource.spotDirection[Etat::curLumi] = 4.0f*glm::normalize( LightSource.spotDirection[Etat::curLumi] );
                // std::cout << " LightSource.spotDirection[Etat::curLumi]=" << glm::to_string(LightSource.spotDirection[Etat::curLumi]) << std::endl;
            }
            break;
        case deplaceLumiPosition:
            {
                // obtenir les coordonnées d'écran correspondant à la position de la lumière
                glm::vec3 ecranLumi = glm::project( glm::vec3(LightSource.position[Etat::curLumi]), V*M, P, cloture );
                // définir la nouvelle position (en utilisant la profondeur actuelle)
                glm::vec3 ecranPos( x, hauteur_-y, ecranLumi[2] );
                // placer la lumière à cette nouvelle position
                LightSource.position[Etat::curLumi] = glm::vec4( glm::unProject( ecranPos, V*M, P, cloture ), 1.0 );
                // std::cout << " LightSource.position[Etat::curLumi]=" << glm::to_string(LightSource.position[Etat::curLumi]) << std::endl;
            }
            break;
        }

        Etat::sourisPosPrec.x = x;
        Etat::sourisPosPrec.y = y;

        camera.verifierAngles();
    }
}

int main( int argc, char *argv[] )
{
    // créer une fenêtre
    FenetreTP fenetre( "INF2705 TP" );

    // allouer des ressources et définir le contexte OpenGL
    fenetre.initialiser();

    bool boucler = true;
    while ( boucler )
    {
        // mettre à jour la physique
        calculerPhysique( );

        // affichage
        fenetre.afficherScene();

        // récupérer les événements et appeler la fonction de rappel
        boucler = fenetre.gererEvenement();
    }

    // détruire les ressources OpenGL allouées
    fenetre.conclure();

    return 0;
}

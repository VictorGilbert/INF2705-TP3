#version 410

// Définition des paramètres des sources de lumière
layout (std140) uniform LightSourceParameters
{
    vec4 ambient[3];
    vec4 diffuse[3];
    vec4 specular[3];
    vec4 position[3];      // dans le repère du monde
    vec3 spotDirection[3]; // dans le repère du monde
    float spotExponent;
    float spotAngleOuverture; // ([0.0,90.0] ou 180.0)
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
} LightSource;

// Définition des paramètres des matériaux
layout (std140) uniform MaterialParameters
{
    vec4 emission;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    float shininess;
} FrontMaterial;

// Définition des paramètres globaux du modèle de lumière
layout (std140) uniform LightModelParameters
{
    vec4 ambient;       // couleur ambiante globale
    bool twoSide;       // éclairage sur les deux côtés ou un seul?
} LightModel;

layout (std140) uniform varsUnif
{
    // partie 1: illumination
    int typeIllumination;     // 0:Gouraud, 1:Phong
    bool utiliseSpot;         // indique si on utilise des lumière de type spot ou point
    bool utiliseBlinn;        // indique si on veut utiliser modèle spéculaire de Blinn ou Phong
    bool utiliseDirect;       // indique si on utilise un spot style Direct3D ou OpenGL
    bool afficheNormales;     // indique si on utilise les normales comme couleurs (utile pour le débogage)
    // partie 2: texture
    float tempsGlissement;    // temps de glissement
    int iTexCoul;             // numéro de la texture de couleurs appliquée
    // partie 3b: texture
    int iTexNorm;             // numéro de la texture de normales appliquée
};

uniform sampler2D laTextureCoul;
uniform sampler2D laTextureNorm;


/////////////////////////////////////////////////////////////////

in Attribs {
    vec4 couleur;
    vec2 texCoord;
} AttribsIn;

out vec4 FragColor;


void main( void )
{
    // ...
    vec4 coul = AttribsIn.couleur; // la composante ambiante déjà calculée (dans nuanceur de sommets)

    FragColor = clamp(coul, 0.0, 1.0);

    // Pour « voir » les normales, on peut remplacer la couleur du fragment par la normale.
    // (Les composantes de la normale variant entre -1 et +1, il faut
    // toutefois les convertir en une couleur entre 0 et +1 en faisant (N+1)/2.)
    //if ( afficheNormales ) FragColor = clamp( vec4( (N+1)/2, AttribsIn.couleur.a ), 0.0, 1.0 );

    // Lorsqu'il y a une texture chargee
    if (iTexCoul != 0) {
        vec4 couleurTexture = texture( laTextureCoul, AttribsIn.texCoord.xy - vec2(tempsGlissement, 0) );
        // Transparence quand le texel est trop sombre
        if(length(couleurTexture.rgb) < 0.5) {
            discard;
        }
        FragColor *= couleurTexture;
    }
}

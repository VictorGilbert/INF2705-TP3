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

uniform mat4 matrModel;
uniform mat4 matrVisu;
uniform mat4 matrProj;
uniform mat3 matrNormale;

/////////////////////////////////////////////////////////////////

layout(location=0) in vec4 Vertex;
layout(location=2) in vec3 Normal;
layout(location=8) in vec2 TexCoord;
//layout(location=2) in vec2 TexNorm;
//layout(location = 3) in vec4 Color;

out Attribs {
    vec4 couleur;
    vec2 texCoord;
} AttribsOut;


float calculerSpot(in vec3 D, in vec3 L, in vec3 N)
{
    float spotFacteur = 0.0;

    if (dot(D, N) >= 0)
    {
        float spotDot = dot(L, D);
        float cosInner = cos(radians(LightSource.spotAngleOuverture));
        float cosOuter = pow(cosInner, 1.01 + LightSource.spotExponent / 2);
        if (utiliseDirect) {
            spotFacteur = smoothstep(cosOuter, cosInner, spotDot);
        }
        else if (spotDot > cosInner) {
            spotFacteur = pow(spotDot, LightSource.spotExponent);
        }
    }

    return(spotFacteur);
}

float attenuation = 1.0;
vec4 calculerReflexion( in int j, in vec3 L, in vec3 N, in vec3 O ) // pour la lumière j
{
    vec4 coul = vec4(0);
    coul += FrontMaterial.ambient * LightSource.ambient[j];

    // calculer l'éclairage seulement si le produit scalaire est positif
    float NdotL = dot( N, L );
    if ( NdotL > 0.0 )
    {
        // calculer la composante diffuse
        coul += attenuation * FrontMaterial.diffuse * LightSource.diffuse[j] * NdotL;
        //vec4(Color.x, Color.y, Color.z, 1)
        // calculer la composante spéculaire (Blinn ou Phong : spec = BdotN ou RdotO )
        float spec = ( utiliseBlinn ?
                       dot( normalize( L + O ), N ) : // dot( B, N )
                       dot( reflect( -L, N ), O ) ); // dot( R, O )
        if ( spec > 0 ) coul += attenuation * FrontMaterial.specular * LightSource.specular[j] * pow( spec, FrontMaterial.shininess );
    }

    return( coul );
}

void main( void )
{
    // Nuanceur inspiré des exemples du cours, et code d'Antoine Soldati et Etienne Perron
    // appliquer la transformation standard du sommet (P * V * M * sommet)
    gl_Position = matrProj * matrVisu * matrModel * Vertex;

    // calcul de la composante ambiante du modèle
    vec4 coul = FrontMaterial.emission + FrontMaterial.ambient * LightModel.ambient;
    
        // calculer la normale (N) qui sera interpolée pour le nuanceur de fragments
    vec3 N = normalize(matrNormale * Normal);

    // calculer la position (P) du sommet (dans le repère de la caméra)
    vec3 pos = vec3( matrVisu * matrModel * Vertex );
    vec3 obsVec = -pos ; // on considère que l'observateur (la caméra) est à l'infini dans la direction (0,0,1)


    vec3 O = normalize( obsVec );  // position de l'observateur
    float[3] spots;

    // calculer le vecteur de la direction (L) de la lumière (dans le repère de la caméra)
    // calculer la réflexion
    for (int j = 0; j < 3; j++){
        vec3 lightVec = ( matrVisu * LightSource.position[j] ).xyz - pos;
        vec3 L = normalize( lightVec ); // vecteur vers la source lumineuse
        coul += calculerReflexion(j, L, N, O);
        //AttribsOut.spotDirection[i] = mat3(matrVisu) * -LightSource.spotDirection[i];
        if (utiliseSpot) {
            vec3 D = normalize(mat3(matrVisu) * -LightSource.spotDirection[j]);
            spots[j] = calculerSpot(D, L, N);
        }
    }
    //On applique le spot pour chaque couleur
    if (utiliseSpot) {
        coul.x *= spots[0];
        coul.y *= spots[1];
        coul.z *= spots[2];
    }
    AttribsOut.couleur = clamp( coul, 0.0, 1.0 );
    AttribsOut.texCoord = TexCoord.xy;
}

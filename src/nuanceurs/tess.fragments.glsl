#version 410

/////////////////////////////////////////////////////////////////

in Attribs{
    vec4 couleur;
    vec2 texCoord;
} AttribsIn;

out vec4 FragColor;

//uniform sampler2D heightMapTex;

void main( void )
{
    FragColor = AttribsIn.couleur;
    //FragColor = AttribsIn.couleur * 0 + texture(heightMapTex, AttribsIn.texCoord.xy);
    //FragColor //= vec4(1, 0, 1, 1);
    //= texture(heightMapTex, AttribsIn.texCoord.xy);
}

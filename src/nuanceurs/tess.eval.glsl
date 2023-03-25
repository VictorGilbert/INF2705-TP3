#version 410

// ==> Les variables en commentaires ci-dessous sont déclarées implicitement:
// in vec3 gl_TessCoord;
// in int gl_PatchVerticesIn;
// in int gl_PrimitiveID;
// patch in float gl_TessLevelOuter[4];
// patch in float gl_TessLevelInner[2];
// in gl_PerVertex
// {
//   vec4 gl_Position;
//   float gl_PointSize;
//   float gl_ClipDistance[];
// } gl_in[gl_MaxPatchVertices];

// out gl_PerVertex
// {
//   vec4 gl_Position;
//   float gl_PointSize;
//   float gl_ClipDistance[];
// };

layout(quads) in;

in Attribs{
    vec2 texCoord;
} AttribsIn[];

out Attribs{
    vec4 couleur;
} AttribsOut;

uniform mat4 matrModel;

uniform sampler2D heightMapTex;

vec4 interpole(vec4 v0, vec4 v1, vec4 v2, vec4 v3)
{
    vec4 v01 = mix(v0, v1, gl_TessCoord.x);
    vec4 v32 = mix(v3, v2, gl_TessCoord.x);
    return mix(v01, v32, gl_TessCoord.y);
}

void main()
{
    // interpoler la position et les attributs selon gl_TessCoord
    vec4 coul = texture(heightMapTex, gl_TessCoord.xy);
    vec4 pos = interpole(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position, gl_in[3].gl_Position);
    pos.y += 10 * coul.r;
    
    gl_Position = pos;
    
    AttribsOut.couleur = coul;
}

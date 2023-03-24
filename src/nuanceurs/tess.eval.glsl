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
    vec2 texCoord;  //
    vec4 couleur; //
} AttribsIn[];

out Attribs{
    vec4 couleur;
    vec2 texCoord;
} AttribsOut;

uniform mat4 matrModel;
uniform bool deformer;

uniform sampler2D heightMapTex;

vec4 interpole(vec4 v0, vec4 v1, vec4 v2, vec4 v3)
{
    // mix( x, y, f ) = x * (1-f) + y * f.
    vec4 v01 = mix(v0, v1, gl_TessCoord.x);
    vec4 v32 = mix(v3, v2, gl_TessCoord.x);
    return mix(v01, v32, gl_TessCoord.y);
}


void main()
{
    // interpoler la position et les attributs selon gl_TessCoord
    vec4 coul = texture(heightMapTex, gl_TessCoord.xy);
    vec4 pos = interpole(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position, gl_in[3].gl_Position);
    pos.y += 10 * max(coul.r, 0);
    
    gl_Position = pos;
    //AttribsOut.couleur = interpole(AttribsIn[0].couleur, AttribsIn[1].couleur, AttribsIn[2].couleur);
    //AttribsOut.couleur = interpole(AttribsIn[0].couleur, AttribsIn[1].couleur, AttribsIn[2].couleur, AttribsIn[3].couleur);
    
    
    //AttribsOut.couleur = AttribsIn[gl_PrimitiveID].couleur;
    AttribsOut.couleur = coul;
    AttribsOut.texCoord = gl_TessCoord.xy;

    // on déforme la primitive et on multiplie par matrModel qui n'avait pas été fait
    if (deformer)
    {
        gl_Position.z = 1.0 - length(gl_Position.xy);
        gl_Position = matrModel * gl_Position;
    }
}

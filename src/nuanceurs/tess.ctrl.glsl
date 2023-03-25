#version 410

// ==> Les variables en commentaires ci-dessous sont d�clar�es implicitement:
// in int gl_PatchVerticesIn;
// in int gl_PrimitiveID;
// in int gl_InvocationID;
// in gl_PerVertex
// {
//   vec4 gl_Position;
//   float gl_PointSize;
//   float gl_ClipDistance[];
// } gl_in[gl_MaxPatchVertices];

// patch out float gl_TessLevelOuter[4];
// patch out float gl_TessLevelInner[2];
// out gl_PerVertex
// {
//   vec4 gl_Position;
//   float gl_PointSize;
//   float gl_ClipDistance[];
// } gl_out[];

// ... 4??
layout(vertices = 4) out;

uniform float TessLevelInner;
uniform float TessLevelOuter;

in Attribs{
    vec2 texCoord;
} AttribsIn[];

out Attribs{
    vec2 texCoord;
} AttribsOut[];

void main()
{
    // copier la position du sommet vers la sortie
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

    // donner les niveaux de tessellation d�sir�e (on le fait seuleemnt pour la premi�re invocation)
    if (gl_InvocationID == 0)
    {
        // ... ???
        gl_TessLevelInner[0] = TessLevelInner;
        gl_TessLevelInner[1] = TessLevelInner;
        gl_TessLevelOuter[0] = TessLevelOuter;
        gl_TessLevelOuter[1] = TessLevelOuter;
        gl_TessLevelOuter[2] = TessLevelOuter;
        gl_TessLevelOuter[3] = TessLevelOuter;
    }

    // copier les autres attributs vers la sortie
    AttribsOut[gl_InvocationID].texCoord = AttribsIn[gl_InvocationID].texCoord;
}

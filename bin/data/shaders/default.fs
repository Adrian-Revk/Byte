#version 150
#extension GL_ARB_separate_shader_objects : enable

in vec2 texCoords;

out vec4 outColor;

uniform sampler2D Albedo;

void main() {
    vec4 oColor = texture( Albedo, texCoords );
    if( oColor.a < 0.5 )
        discard;
    outColor = oColor;
}


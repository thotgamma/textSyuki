#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vert;

void main(){

    gl_Position = vec4(vert.x, vert.y, 0.0, 1.0);

}


# ESL

```glsl

attributes
{
    in vec4 vertexPosition;

    @condition(hasVertexNormals)
    {
        in vec4 vertexNormal;
        @condition(hasVertexTangents)
        {
            in vec4 vertexTangent; 
        }
    }

    @condition(hasVertexColors)
    {
        in vec4 vertexColor;
    }

    out vec4 fragmentPosition;
    out vec4 fragmentNormal;
    out vec4 fragmentColor;
}

buffer entity
{

}

shader 
{

}

```




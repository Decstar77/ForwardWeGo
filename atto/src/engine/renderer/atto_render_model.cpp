#include "atto_render_model.h"
#include "../atto_log.h"

#include <glad/glad.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <assimp/texture.h>
#include <assimp/mesh.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/std_image.h>

namespace atto {

    void Texture::LoadFromFile( const char * filePath ) {
        int width, height, channels;
        stbi_uc * data = stbi_load( filePath, &width, &height, &channels, STBI_rgb_alpha );
        if ( !data ) {
            LOG_ERROR( "Failed to load texture image '%s'", filePath );
            width = height = 0;
            texture = 0;
            return;
        }

        this->width = width;
        this->height = height;

        glGenTextures( 1, &texture );
        glBindTexture( GL_TEXTURE_2D, texture );

        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
        glGenerateMipmap( GL_TEXTURE_2D );

        glBindTexture( GL_TEXTURE_2D, 0 );

        stbi_image_free( data );
    }

    void Mesh::Create( const std::vector<Vertex> & vertices, const std::vector<u32> & indices ) {
        indexCount = static_cast<i32>(indices.size());

        glGenVertexArrays( 1, &vao );
        glGenBuffers( 1, &vbo );
        glGenBuffers( 1, &ebo );

        glBindVertexArray( vao );

        glBindBuffer( GL_ARRAY_BUFFER, vbo );
        glBufferData( GL_ARRAY_BUFFER, vertices.size() * sizeof( Vertex ), vertices.data(), GL_STATIC_DRAW );

        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ebo );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof( u32 ), indices.data(), GL_STATIC_DRAW );

        // Position (location = 0)
        glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( Vertex ), (void *)offsetof( Vertex, position ) );
        glEnableVertexAttribArray( 0 );

        // Normal (location = 1)
        glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof( Vertex ), (void *)offsetof( Vertex, normal ) );
        glEnableVertexAttribArray( 1 );

        // TexCoords (location = 2)
        glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, sizeof( Vertex ), (void *)offsetof( Vertex, texCoords ) );
        glEnableVertexAttribArray( 2 );

        glBindVertexArray( 0 );
    }

    void Mesh::Destroy() {
        if ( ebo != 0 ) { glDeleteBuffers( 1, &ebo ); ebo = 0; }
        if ( vbo != 0 ) { glDeleteBuffers( 1, &vbo ); vbo = 0; }
        if ( vao != 0 ) { glDeleteVertexArrays( 1, &vao ); vao = 0; }
        indexCount = 0;
    }

    void Mesh::Draw() const {
        glBindVertexArray( vao );
        glDrawElements( GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0 );
        glBindVertexArray( 0 );
    }

    static Mesh ProcessMesh( aiMesh * mesh, const aiScene * scene, f32 scale ) {
        std::vector<Vertex> vertices;
        std::vector<u32> indices;

        vertices.reserve( mesh->mNumVertices );
        for ( u32 i = 0; i < mesh->mNumVertices; i++ ) {
            Vertex vertex = {};

            vertex.position.x = mesh->mVertices[i].x * scale;
            vertex.position.y = mesh->mVertices[i].y * scale;
            vertex.position.z = mesh->mVertices[i].z * scale;

            if ( mesh->HasNormals() ) {
                vertex.normal.x = mesh->mNormals[i].x;
                vertex.normal.y = mesh->mNormals[i].y;
                vertex.normal.z = mesh->mNormals[i].z;
            }

            if ( mesh->mTextureCoords[0] ) {
                vertex.texCoords.x = mesh->mTextureCoords[0][i].x;
                vertex.texCoords.y = mesh->mTextureCoords[0][i].y;
            }

            vertices.push_back( vertex );
        }

        for ( u32 i = 0; i < mesh->mNumFaces; i++ ) {
            aiFace & face = mesh->mFaces[i];
            for ( u32 j = 0; j < face.mNumIndices; j++ ) {
                indices.push_back( face.mIndices[j] );
            }
        }

        Mesh result;
        result.Create( vertices, indices );
        return result;
    }

    static void PrintMaterials( const aiScene * scene ) {
        for ( u32 i = 0; i < scene->mNumMaterials; i++ ) {
            const aiMaterial * material = scene->mMaterials[i];
            aiString name;
            material->Get( AI_MATKEY_NAME, name );
            printf( "Material: %s\n", name.C_Str() );

            // Print Diffuse texture(s)
            u32 numDiffuse = material->GetTextureCount( aiTextureType_DIFFUSE );
            for ( u32 t = 0; t < numDiffuse; ++t ) {
                aiString texPath;
                if ( material->GetTexture( aiTextureType_DIFFUSE, t, &texPath ) == AI_SUCCESS ) {
                    printf( "  Diffuse Texture: %s\n", texPath.C_Str() );
                }
            }

            // Print Specular texture(s)
            u32 numSpecular = material->GetTextureCount( aiTextureType_SPECULAR );
            for ( u32 t = 0; t < numSpecular; ++t ) {
                aiString texPath;
                if ( material->GetTexture( aiTextureType_SPECULAR, t, &texPath ) == AI_SUCCESS ) {
                    printf( "  Specular Texture: %s\n", texPath.C_Str() );
                }
            }

            // Print Normal/Bump texture(s)
            u32 numNormals = material->GetTextureCount( aiTextureType_NORMALS );
            for ( u32 t = 0; t < numNormals; ++t ) {
                aiString texPath;
                if ( material->GetTexture( aiTextureType_NORMALS, t, &texPath ) == AI_SUCCESS ) {
                    printf( "  Normal Texture: %s\n", texPath.C_Str() );
                }
            }
            u32 numBump = material->GetTextureCount( aiTextureType_HEIGHT );
            for ( u32 t = 0; t < numBump; ++t ) {
                aiString texPath;
                if ( material->GetTexture( aiTextureType_HEIGHT, t, &texPath ) == AI_SUCCESS ) {
                    printf( "  Bump Map: %s\n", texPath.C_Str() );
                }
            }
        }
    }

    static void ProcessNode( aiNode * node, const aiScene * scene, std::vector<Mesh> & meshes, f32 scale ) {
        for ( u32 i = 0; i < node->mNumMeshes; i++ ) {
            aiMesh * mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back( ProcessMesh( mesh, scene, scale ) );
        }

        for ( u32 i = 0; i < node->mNumChildren; i++ ) {
            ProcessNode( node->mChildren[i], scene, meshes, scale );
        }
    }

    void StaticModel::LoadFromFile( const char * filePath, f32 scale ) {
        Assimp::Importer importer;

        const aiScene * scene = importer.ReadFile( filePath,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs |
            aiProcess_CalcTangentSpace
        );

        if ( !scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode ) {
            LOG_ERROR( "Assimp: %s", importer.GetErrorString() );
            return;
        }

        PrintMaterials( scene );
        ProcessNode( scene->mRootNode, scene, meshes, scale );

        LOG_INFO( "Loaded model '%s' (%d meshes)", filePath, GetMeshCount() );
    }

    void StaticModel::CreateFromMesh( const std::vector<Vertex> & vertices, const std::vector<u32> & indices ) {
        Destroy();
        Mesh mesh;
        mesh.Create( vertices, indices );
        meshes.push_back( std::move( mesh ) );
    }

    void StaticModel::Destroy() {
        for ( auto & mesh : meshes ) {
            mesh.Destroy();
        }
        meshes.clear();
    }

    void StaticModel::Draw() const {
        for ( const auto & mesh : meshes ) {
            mesh.Draw();
        }
    }

    void Brush::ToStaticModel( StaticModel & model ) const {
        const Vec3 & c = center;
        const Vec3 & h = halfExtents;

        // 6 faces, 4 vertices each = 24 vertices, 6 indices each = 36 indices
        std::vector<Vertex> vertices( 24 );
        std::vector<u32> indices( 36 );

        auto setFace = [&]( i32 face, Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3, Vec3 n ) {
            i32 base = face * 4;
            vertices[base + 0] = { p0, n, Vec2( 0.0f, 0.0f ) };
            vertices[base + 1] = { p1, n, Vec2( 1.0f, 0.0f ) };
            vertices[base + 2] = { p2, n, Vec2( 1.0f, 1.0f ) };
            vertices[base + 3] = { p3, n, Vec2( 0.0f, 1.0f ) };

            i32 idx = face * 6;
            indices[idx + 0] = base + 0;
            indices[idx + 1] = base + 1;
            indices[idx + 2] = base + 2;
            indices[idx + 3] = base + 0;
            indices[idx + 4] = base + 2;
            indices[idx + 5] = base + 3;
        };

        // +Z face (front)
        setFace( 0,
            c + Vec3( -h.x, -h.y, +h.z ),
            c + Vec3( +h.x, -h.y, +h.z ),
            c + Vec3( +h.x, +h.y, +h.z ),
            c + Vec3( -h.x, +h.y, +h.z ),
            Vec3( 0, 0, 1 )
        );
        // -Z face (back)
        setFace( 1,
            c + Vec3( +h.x, -h.y, -h.z ),
            c + Vec3( -h.x, -h.y, -h.z ),
            c + Vec3( -h.x, +h.y, -h.z ),
            c + Vec3( +h.x, +h.y, -h.z ),
            Vec3( 0, 0, -1 )
        );
        // +X face (right)
        setFace( 2,
            c + Vec3( +h.x, -h.y, +h.z ),
            c + Vec3( +h.x, -h.y, -h.z ),
            c + Vec3( +h.x, +h.y, -h.z ),
            c + Vec3( +h.x, +h.y, +h.z ),
            Vec3( 1, 0, 0 )
        );
        // -X face (left)
        setFace( 3,
            c + Vec3( -h.x, -h.y, -h.z ),
            c + Vec3( -h.x, -h.y, +h.z ),
            c + Vec3( -h.x, +h.y, +h.z ),
            c + Vec3( -h.x, +h.y, -h.z ),
            Vec3( -1, 0, 0 )
        );
        // +Y face (top)
        setFace( 4,
            c + Vec3( -h.x, +h.y, +h.z ),
            c + Vec3( +h.x, +h.y, +h.z ),
            c + Vec3( +h.x, +h.y, -h.z ),
            c + Vec3( -h.x, +h.y, -h.z ),
            Vec3( 0, 1, 0 )
        );
        // -Y face (bottom)
        setFace( 5,
            c + Vec3( -h.x, -h.y, -h.z ),
            c + Vec3( +h.x, -h.y, -h.z ),
            c + Vec3( +h.x, -h.y, +h.z ),
            c + Vec3( -h.x, -h.y, +h.z ),
            Vec3( 0, -1, 0 )
        );

        model.CreateFromMesh( vertices, indices );
    }

    void Brush::Serialize( Serializer & serializer ) {
        serializer( "center", center );
        serializer( "halfExtents", halfExtents );
    }

} // namespace atto

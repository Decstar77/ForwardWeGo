#include "atto_render_model.h"
#include "../atto_log.h"

#include <glad/glad.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace atto {

    void Mesh::Create( const std::vector<Vertex> & vertices, const std::vector<u32> & indices ) {
        indexCount = static_cast<i32>( indices.size() );

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

    void StaticModel::LoadFromFile( const char * filePath ) {
        Assimp::Importer importer;

        const aiScene * scene = importer.ReadFile( filePath,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs |
            aiProcess_CalcTangentSpace
        );

        if ( !scene || ( scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ) || !scene->mRootNode ) {
            LOG_ERROR( "Assimp: %s", importer.GetErrorString() );
            return;
        }

        ProcessNode( scene->mRootNode, scene );

        LOG_INFO( "Loaded model '%s' (%d meshes)", filePath, GetMeshCount() );
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

    void StaticModel::ProcessNode( aiNode * node, const aiScene * scene ) {
        for ( u32 i = 0; i < node->mNumMeshes; i++ ) {
            aiMesh * mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back( ProcessMesh( mesh, scene ) );
        }

        for ( u32 i = 0; i < node->mNumChildren; i++ ) {
            ProcessNode( node->mChildren[i], scene );
        }
    }

    Mesh StaticModel::ProcessMesh( aiMesh * mesh, const aiScene * scene ) {
        std::vector<Vertex> vertices;
        std::vector<u32> indices;

        vertices.reserve( mesh->mNumVertices );
        for ( u32 i = 0; i < mesh->mNumVertices; i++ ) {
            Vertex vertex = {};

            vertex.position.x = mesh->mVertices[i].x;
            vertex.position.y = mesh->mVertices[i].y;
            vertex.position.z = mesh->mVertices[i].z;

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

} // namespace atto

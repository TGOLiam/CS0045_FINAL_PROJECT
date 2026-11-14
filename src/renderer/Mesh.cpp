#include "Mesh.h"

#include <limits>

bool Mesh::upload(const std::vector<GLfloat>& positions, GLenum primitive)
{
    // Reject incomplete vertices, unsupported primitives, and repeated uploads.
    if (vbo_ || positions.empty() || positions.size() % 3 != 0 ||
        positions.size() / 3 > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()) ||
        positions.size() > static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max()) / sizeof(GLfloat) ||
        (primitive != GL_TRIANGLES && primitive != GL_LINES && primitive != GL_POINTS)) {
        return false;
    }

    // Restore the caller's buffer binding after uploading static geometry.
    GLint previousBuffer = 0;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousBuffer);
    glGenBuffers(1, &vbo_);
    if (!vbo_) return false;
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(positions.size() * sizeof(GLfloat)),
                 positions.data(), GL_STATIC_DRAW);
    // Check that OpenGL allocated the requested buffer before accepting it.
    GLint allocated = 0;
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &allocated);
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(previousBuffer));
    if (static_cast<GLsizeiptr>(allocated) != static_cast<GLsizeiptr>(positions.size() * sizeof(GLfloat))) {
        release();
        return false;
    }
    primitive_ = primitive;
    vertexCount_ = static_cast<GLsizei>(positions.size() / 3);
    return true;
}

bool Mesh::uploadDynamic(const std::vector<GLfloat>& positions, GLenum primitive)
{
    // Per-frame refreshes: re-specify the buffer as dynamic so growing or
    // shrinking vertex counts are valid, and keep vertexCount_ accurate.
    if (positions.empty() || positions.size() % 3 != 0 ||
        positions.size() / 3 > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()) ||
        positions.size() > static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max()) / sizeof(GLfloat) ||
        (primitive != GL_TRIANGLES && primitive != GL_LINES && primitive != GL_POINTS)) {
        return false;
    }
    GLint previousBuffer = 0;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousBuffer);
    if (!vbo_) {
        glGenBuffers(1, &vbo_);
        if (!vbo_) return false;
    } else if (primitive_ != primitive) {
        return false;
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(positions.size() * sizeof(GLfloat)),
                 positions.data(), GL_DYNAMIC_DRAW);
    GLint allocated = 0;
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &allocated);
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(previousBuffer));
    if (static_cast<GLsizeiptr>(allocated) != static_cast<GLsizeiptr>(positions.size() * sizeof(GLfloat))) {
        vertexCount_ = 0;
        return false;
    }
    primitive_ = primitive;
    vertexCount_ = static_cast<GLsizei>(positions.size() / 3);
    return true;
}

void Mesh::draw() const
{
    drawRange(0, vertexCount_);
}

void Mesh::drawRange(GLint first, GLsizei count) const
{
    if (!vbo_ || first < 0 || count <= 0 || first > vertexCount_ || count > vertexCount_ - first) return;
    // Preserve client-side array state and its buffer binding for the caller.
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glEnableClientState(GL_VERTEX_ARRAY);
    // With a VBO bound, nullptr means byte offset zero within that buffer.
    glVertexPointer(3, GL_FLOAT, 0, nullptr);
    glDrawArrays(primitive_, first, count);
    glPopClientAttrib();
}

void Mesh::release()
{
    if (vbo_) glDeleteBuffers(1, &vbo_);
    vbo_ = 0;
    vertexCount_ = 0;
}

/*
    Open1560 - An Open Source Re-Implementation of Midtown Madness 1 Beta
    Copyright (C) 2020 Brick

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

// GLES adjustments to what glad loaded, plus an audit of what is still missing.
//
// glad loads the desktop GL API and gates each entry point on the desktop
// version it belongs to. A GLES 3.0 context reports version 3.0, so anything
// desktop GL only added later never gets resolved - even though GLES 3.0
// provides it (texture storage, sync objects, program binaries). Those are
// filled in here by name.
//
// What remains missing is genuinely absent: the fixed-function entry points and
// the desktop-only buffer and state calls. Calling one is a jump to address
// zero, so the audit lists them at startup.

#include <glad/glad.h>

#ifdef __ANDROID__

#    include <SDL3/SDL_video.h>
#    include <android/log.h>

namespace
{
    void* Resolve(const char* name)
    {
        return reinterpret_cast<void*>(SDL_GL_GetProcAddress(name));
    }

    PFNGLCLEARDEPTHFPROC ClearDepthf;

    // GLES spells this one with an f and takes a float.
    void GLAPIENTRY ClearDepthShim(GLdouble depth)
    {
        if (ClearDepthf)
            ClearDepthf(static_cast<GLfloat>(depth));
    }

    // No wireframe or point fill in GLES; the renderer only uses this for the
    // debug fill modes.
    void GLAPIENTRY PolygonModeShim(GLenum, GLenum)
    {}
} // namespace

extern "C" void ArtsLoadGLESEntryPoints()
{
#    define ARTS_LOAD_GL(TYPE, NAME)                        \
        if (glad_##NAME == nullptr)                         \
        {                                                   \
            glad_##NAME = reinterpret_cast<TYPE>(Resolve(#NAME)); \
        }

    // Core in GLES 3.0, but desktop GL 3.2 or later.
    ARTS_LOAD_GL(PFNGLFENCESYNCPROC, glFenceSync)
    ARTS_LOAD_GL(PFNGLCLIENTWAITSYNCPROC, glClientWaitSync)
    ARTS_LOAD_GL(PFNGLDELETESYNCPROC, glDeleteSync)

    // Core in GLES 3.0, but desktop GL 4.2.
    ARTS_LOAD_GL(PFNGLTEXSTORAGE2DPROC, glTexStorage2D)
    ARTS_LOAD_GL(PFNGLGETINTERNALFORMATIVPROC, glGetInternalformativ)

    // Core in GLES 3.0, but desktop GL 4.1.
    ARTS_LOAD_GL(PFNGLGETPROGRAMBINARYPROC, glGetProgramBinary)

#    undef ARTS_LOAD_GL

    // Entry points GLES provides under another name, or that have no meaning
    // here and can be dropped.
    if (glad_glClearDepth == nullptr)
    {
        ClearDepthf = reinterpret_cast<PFNGLCLEARDEPTHFPROC>(Resolve("glClearDepthf"));
        glad_glClearDepth = ClearDepthShim;
    }

    if (glad_glPolygonMode == nullptr)
        glad_glPolygonMode = PolygonModeShim;
}

extern "C" void ArtsAuditGLEntryPoints()
{
    struct Entry
    {
        const char* Name;
        const void* Proc;
    };

    // Every gl* call in agi/ and agigl/.
    static const Entry entries[] {
        {"glActiveTexture", reinterpret_cast<const void*>(glad_glActiveTexture)},
        {"glAlphaFunc", reinterpret_cast<const void*>(glad_glAlphaFunc)},
        {"glAttachShader", reinterpret_cast<const void*>(glad_glAttachShader)},
        {"glBegin", reinterpret_cast<const void*>(glad_glBegin)},
        {"glBindAttribLocation", reinterpret_cast<const void*>(glad_glBindAttribLocation)},
        {"glBindBuffer", reinterpret_cast<const void*>(glad_glBindBuffer)},
        {"glBindFramebuffer", reinterpret_cast<const void*>(glad_glBindFramebuffer)},
        {"glBindRenderbuffer", reinterpret_cast<const void*>(glad_glBindRenderbuffer)},
        {"glBindTexture", reinterpret_cast<const void*>(glad_glBindTexture)},
        {"glBindTextureUnit", reinterpret_cast<const void*>(glad_glBindTextureUnit)},
        {"glBindVertexArray", reinterpret_cast<const void*>(glad_glBindVertexArray)},
        {"glBlendFunc", reinterpret_cast<const void*>(glad_glBlendFunc)},
        {"glBlitFramebuffer", reinterpret_cast<const void*>(glad_glBlitFramebuffer)},
        {"glBufferData", reinterpret_cast<const void*>(glad_glBufferData)},
        {"glBufferStorage", reinterpret_cast<const void*>(glad_glBufferStorage)},
        {"glBufferSubData", reinterpret_cast<const void*>(glad_glBufferSubData)},
        {"glCheckFramebufferStatus", reinterpret_cast<const void*>(glad_glCheckFramebufferStatus)},
        {"glClear", reinterpret_cast<const void*>(glad_glClear)},
        {"glClearColor", reinterpret_cast<const void*>(glad_glClearColor)},
        {"glClearDepth", reinterpret_cast<const void*>(glad_glClearDepth)},
        {"glClientWaitSync", reinterpret_cast<const void*>(glad_glClientWaitSync)},
        {"glClipControl", reinterpret_cast<const void*>(glad_glClipControl)},
        {"glColor4ub", reinterpret_cast<const void*>(glad_glColor4ub)},
        {"glCompileShader", reinterpret_cast<const void*>(glad_glCompileShader)},
        {"glCreateProgram", reinterpret_cast<const void*>(glad_glCreateProgram)},
        {"glCreateShader", reinterpret_cast<const void*>(glad_glCreateShader)},
        {"glDebugMessageCallback", reinterpret_cast<const void*>(glad_glDebugMessageCallback)},
        {"glDebugMessageControl", reinterpret_cast<const void*>(glad_glDebugMessageControl)},
        {"glDeleteBuffers", reinterpret_cast<const void*>(glad_glDeleteBuffers)},
        {"glDeleteFramebuffers", reinterpret_cast<const void*>(glad_glDeleteFramebuffers)},
        {"glDeleteProgram", reinterpret_cast<const void*>(glad_glDeleteProgram)},
        {"glDeleteRenderbuffers", reinterpret_cast<const void*>(glad_glDeleteRenderbuffers)},
        {"glDeleteShader", reinterpret_cast<const void*>(glad_glDeleteShader)},
        {"glDeleteSync", reinterpret_cast<const void*>(glad_glDeleteSync)},
        {"glDeleteTextures", reinterpret_cast<const void*>(glad_glDeleteTextures)},
        {"glDeleteVertexArrays", reinterpret_cast<const void*>(glad_glDeleteVertexArrays)},
        {"glDepthFunc", reinterpret_cast<const void*>(glad_glDepthFunc)},
        {"glDepthMask", reinterpret_cast<const void*>(glad_glDepthMask)},
        {"glDepthRangedNV", reinterpret_cast<const void*>(glad_glDepthRangedNV)},
        {"glDetachShader", reinterpret_cast<const void*>(glad_glDetachShader)},
        {"glDrawRangeElements", reinterpret_cast<const void*>(glad_glDrawRangeElements)},
        {"glDrawRangeElementsBaseVertex", reinterpret_cast<const void*>(glad_glDrawRangeElementsBaseVertex)},
        {"glEnable", reinterpret_cast<const void*>(glad_glEnable)},
        {"glEnd", reinterpret_cast<const void*>(glad_glEnd)},
        {"glFenceSync", reinterpret_cast<const void*>(glad_glFenceSync)},
        {"glFinish", reinterpret_cast<const void*>(glad_glFinish)},
        {"glFlush", reinterpret_cast<const void*>(glad_glFlush)},
        {"glFlushMappedBufferRange", reinterpret_cast<const void*>(glad_glFlushMappedBufferRange)},
        {"glFogCoordf", reinterpret_cast<const void*>(glad_glFogCoordf)},
        {"glFogf", reinterpret_cast<const void*>(glad_glFogf)},
        {"glFogfv", reinterpret_cast<const void*>(glad_glFogfv)},
        {"glFogi", reinterpret_cast<const void*>(glad_glFogi)},
        {"glFramebufferRenderbuffer", reinterpret_cast<const void*>(glad_glFramebufferRenderbuffer)},
        {"glFrontFace", reinterpret_cast<const void*>(glad_glFrontFace)},
        {"glGenBuffers", reinterpret_cast<const void*>(glad_glGenBuffers)},
        {"glGenFramebuffers", reinterpret_cast<const void*>(glad_glGenFramebuffers)},
        {"glGenRenderbuffers", reinterpret_cast<const void*>(glad_glGenRenderbuffers)},
        {"glGenTextures", reinterpret_cast<const void*>(glad_glGenTextures)},
        {"glGenVertexArrays", reinterpret_cast<const void*>(glad_glGenVertexArrays)},
        {"glGetError", reinterpret_cast<const void*>(glad_glGetError)},
        {"glGetIntegerv", reinterpret_cast<const void*>(glad_glGetIntegerv)},
        {"glGetInternalformativ", reinterpret_cast<const void*>(glad_glGetInternalformativ)},
        {"glGetProgramBinary", reinterpret_cast<const void*>(glad_glGetProgramBinary)},
        {"glGetProgramInfoLog", reinterpret_cast<const void*>(glad_glGetProgramInfoLog)},
        {"glGetProgramiv", reinterpret_cast<const void*>(glad_glGetProgramiv)},
        {"glGetRenderbufferParameteriv", reinterpret_cast<const void*>(glad_glGetRenderbufferParameteriv)},
        {"glGetShaderInfoLog", reinterpret_cast<const void*>(glad_glGetShaderInfoLog)},
        {"glGetShaderiv", reinterpret_cast<const void*>(glad_glGetShaderiv)},
        {"glGetUniformLocation", reinterpret_cast<const void*>(glad_glGetUniformLocation)},
        {"glHint", reinterpret_cast<const void*>(glad_glHint)},
        {"glLinkProgram", reinterpret_cast<const void*>(glad_glLinkProgram)},
        {"glLoadMatrixf", reinterpret_cast<const void*>(glad_glLoadMatrixf)},
        {"glMapBuffer", reinterpret_cast<const void*>(glad_glMapBuffer)},
        {"glMapBufferRange", reinterpret_cast<const void*>(glad_glMapBufferRange)},
        {"glMatrixMode", reinterpret_cast<const void*>(glad_glMatrixMode)},
        {"glPixelStorei", reinterpret_cast<const void*>(glad_glPixelStorei)},
        {"glPolygonMode", reinterpret_cast<const void*>(glad_glPolygonMode)},
        {"glProvokingVertex", reinterpret_cast<const void*>(glad_glProvokingVertex)},
        {"glReadPixels", reinterpret_cast<const void*>(glad_glReadPixels)},
        {"glRenderbufferStorage", reinterpret_cast<const void*>(glad_glRenderbufferStorage)},
        {"glRenderbufferStorageMultisample", reinterpret_cast<const void*>(glad_glRenderbufferStorageMultisample)},
        {"glScissor", reinterpret_cast<const void*>(glad_glScissor)},
        {"glShadeModel", reinterpret_cast<const void*>(glad_glShadeModel)},
        {"glShaderSource", reinterpret_cast<const void*>(glad_glShaderSource)},
        {"glTexCoord2fv", reinterpret_cast<const void*>(glad_glTexCoord2fv)},
        {"glTexEnvi", reinterpret_cast<const void*>(glad_glTexEnvi)},
        {"glTexImage2D", reinterpret_cast<const void*>(glad_glTexImage2D)},
        {"glTexParameterf", reinterpret_cast<const void*>(glad_glTexParameterf)},
        {"glTexParameteri", reinterpret_cast<const void*>(glad_glTexParameteri)},
        {"glTexStorage2D", reinterpret_cast<const void*>(glad_glTexStorage2D)},
        {"glTexSubImage2D", reinterpret_cast<const void*>(glad_glTexSubImage2D)},
        {"glUniform1f", reinterpret_cast<const void*>(glad_glUniform1f)},
        {"glUniform1i", reinterpret_cast<const void*>(glad_glUniform1i)},
        {"glUniform2i", reinterpret_cast<const void*>(glad_glUniform2i)},
        {"glUniform3f", reinterpret_cast<const void*>(glad_glUniform3f)},
        {"glUniform4f", reinterpret_cast<const void*>(glad_glUniform4f)},
        {"glUniform4fv", reinterpret_cast<const void*>(glad_glUniform4fv)},
        {"glUnmapBuffer", reinterpret_cast<const void*>(glad_glUnmapBuffer)},
        {"glUseProgram", reinterpret_cast<const void*>(glad_glUseProgram)},
        {"glVertex4fv", reinterpret_cast<const void*>(glad_glVertex4fv)},
        {"glVertexAttribPointer", reinterpret_cast<const void*>(glad_glVertexAttribPointer)},
        {"glViewport", reinterpret_cast<const void*>(glad_glViewport)},
    };

    int missing = 0;

    for (const Entry& entry : entries)
    {
        if (entry.Proc == nullptr)
        {
            ++missing;
            __android_log_print(ANDROID_LOG_WARN, "Open1560", "[gl-missing] %s", entry.Name);
        }
    }

    __android_log_print(ANDROID_LOG_WARN, "Open1560", "=== %d of %zu GL entry points missing ===", missing,
        sizeof(entries) / sizeof(entries[0]));
}

#endif

/*
 This file was generated with gl_gen.cmake, part of glXXw
 (hosted at https://github.com/paroj/glXXw-cmake)
 This is free and unencumbered software released into the public domain.
 Anyone is free to copy, modify, publish, use, compile, sell, or
 distribute this software, either in source code form or as a compiled
 binary, for any purpose, commercial or non-commercial, and by any
 means.
 In jurisdictions that recognize copyright laws, the author or authors
 of this software dedicate any and all copyright interest in the
 software to the public domain. We make this dedication for the benefit
 of the public at large and to the detriment of our heirs and
 successors. We intend this dedication to be an overt act of
 relinquishment in perpetuity of all present and future rights to this
 software under copyright law.
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.
 */

//#ifdef ENABLE_GLEXT_FUNC

#include "glesw.h"
#include <stdio.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <EGL/egl.h>

static HMODULE libgl;

static void open_libgl(void)
{
    libgl = LoadLibraryA("libGLESv2.dll");
}

static void close_libgl(void)
{
    FreeLibrary(libgl);
}

static glglProc get_proc(const char *proc)
{
    glglProc res = NULL;
    
    res = (glglProc)eglGetProcAddress(proc);
    if (!res)
        res = (glglProc)GetProcAddress(libgl, proc);
    return res;
}
#elif defined(__APPLE__) || defined(__APPLE_CC__)
#include <CoreFoundation/CoreFoundation.h>

CFBundleRef bundle;
CFURLRef bundleURL;

static void open_libgl(void)
{
    bundle = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.opengles")); // we are always linked to OpenGLES.framework statically, so it is already loaded and could be found by id
    assert(bundle != NULL);
    
    CFRetain(bundle);
    bundleURL = CFBundleCopyBundleURL(bundle);
}

static void close_libgl(void)
{
    CFRelease(bundle);
    CFRelease(bundleURL);
}

static GLESWglProc get_proc(const char *proc)
{
    GLESWglProc res = 0;
    
    CFStringRef procname = CFStringCreateWithCString(kCFAllocatorDefault, proc,
                                                     kCFStringEncodingASCII);
    *(void **)(&res) = CFBundleGetFunctionPointerForName(bundle, procname);
    CFRelease(procname);
    return res;
}
#elif defined(__EMSCRIPTEN__)
#include <EGL/egl.h>
static void open_libgl() {}
static void close_libgl() {}
static glglProc get_proc(const char *proc)
{
    return (glglProc)eglGetProcAddress(proc);
}
#else
#include <dlfcn.h>

static void *libgl;

static void open_libgl(void)
{
    libgl = dlopen("libGLESv2.so", RTLD_LAZY | RTLD_GLOBAL);
}

static void close_libgl(void)
{
    dlclose(libgl);
}

static GLESWglProc get_proc(const char *proc)
{
    return dlsym(libgl, proc);
}
#endif

static struct {
    int major, minor;
} version;

static int parse_version(void)
{
//    if (!glGetString)
//        return -1;
//
//    const char* pcVer = (const char*)glGetString(GL_VERSION);
//    sscanf(pcVer, "OpenGL ES %u.%u", &version.major, &version.minor);
//
//    if (version.major < 2)
//        return -1;
    return 0;
}

static void load_procs(glGetProcAddressProc proc);

int gleswInit(void)
{
    open_libgl();
    load_procs(get_proc);
    close_libgl();
    return parse_version();
}

int gleswInit2(glGetProcAddressProc proc)
{
    load_procs(proc);
    return parse_version();
}

int gleswIsSupported(int major, int minor)
{
    if (major < 2)
        return 0;
    if (version.major == major)
        return version.minor >= minor;
    return version.major >= major;
}

GLESWglProc glGetProcAddress(const char *proc)
{
    return get_proc(proc);
}

PFNGLACTIVESHADERPROGRAMEXTPROC                         glActiveShaderProgramEXT;
PFNGLALPHAFUNCQCOMPROC                                  glAlphaFuncQCOM;
PFNGLAPPLYFRAMEBUFFERATTACHMENTCMAAINTELPROC            glApplyFramebufferAttachmentCMAAINTEL;
PFNGLBEGINCONDITIONALRENDERNVPROC                       glBeginConditionalRenderNV;
PFNGLBEGINPERFMONITORAMDPROC                            glBeginPerfMonitorAMD;
PFNGLBEGINPERFQUERYINTELPROC                            glBeginPerfQueryINTEL;
PFNGLBEGINQUERYEXTPROC                                  glBeginQueryEXT;
PFNGLBINDFRAGDATALOCATIONEXTPROC                        glBindFragDataLocationEXT;
PFNGLBINDFRAGDATALOCATIONINDEXEDEXTPROC                 glBindFragDataLocationIndexedEXT;
PFNGLBINDPROGRAMPIPELINEEXTPROC                         glBindProgramPipelineEXT;
PFNGLBINDVERTEXARRAYOESPROC                             glBindVertexArrayOES;
PFNGLBLENDBARRIERKHRPROC                                glBlendBarrierKHR;
PFNGLBLENDBARRIERNVPROC                                 glBlendBarrierNV;
PFNGLBLENDEQUATIONSEPARATEIEXTPROC                      glBlendEquationSeparateiEXT;
PFNGLBLENDEQUATIONSEPARATEIOESPROC                      glBlendEquationSeparateiOES;
PFNGLBLENDEQUATIONIEXTPROC                              glBlendEquationiEXT;
PFNGLBLENDEQUATIONIOESPROC                              glBlendEquationiOES;
PFNGLBLENDFUNCSEPARATEIEXTPROC                          glBlendFuncSeparateiEXT;
PFNGLBLENDFUNCSEPARATEIOESPROC                          glBlendFuncSeparateiOES;
PFNGLBLENDFUNCIEXTPROC                                  glBlendFunciEXT;
PFNGLBLENDFUNCIOESPROC                                  glBlendFunciOES;
PFNGLBLENDPARAMETERINVPROC                              glBlendParameteriNV;
PFNGLBLITFRAMEBUFFERANGLEPROC                           glBlitFramebufferANGLE;
PFNGLBLITFRAMEBUFFERNVPROC                              glBlitFramebufferNV;
PFNGLBUFFERSTORAGEEXTPROC                               glBufferStorageEXT;
PFNGLCLEARPIXELLOCALSTORAGEUIEXTPROC                    glClearPixelLocalStorageuiEXT;
PFNGLCLIENTWAITSYNCAPPLEPROC                            glClientWaitSyncAPPLE;
PFNGLCOLORMASKIEXTPROC                                  glColorMaskiEXT;
PFNGLCOLORMASKIOESPROC                                  glColorMaskiOES;
PFNGLCOMPRESSEDTEXIMAGE3DOESPROC                        glCompressedTexImage3DOES;
PFNGLCOMPRESSEDTEXSUBIMAGE3DOESPROC                     glCompressedTexSubImage3DOES;
PFNGLCOPYBUFFERSUBDATANVPROC                            glCopyBufferSubDataNV;
PFNGLCOPYIMAGESUBDATAEXTPROC                            glCopyImageSubDataEXT;
PFNGLCOPYIMAGESUBDATAOESPROC                            glCopyImageSubDataOES;
PFNGLCOPYPATHNVPROC                                     glCopyPathNV;
PFNGLCOPYTEXSUBIMAGE3DOESPROC                           glCopyTexSubImage3DOES;
PFNGLCOPYTEXTURELEVELSAPPLEPROC                         glCopyTextureLevelsAPPLE;
PFNGLCOVERFILLPATHINSTANCEDNVPROC                       glCoverFillPathInstancedNV;
PFNGLCOVERFILLPATHNVPROC                                glCoverFillPathNV;
PFNGLCOVERSTROKEPATHINSTANCEDNVPROC                     glCoverStrokePathInstancedNV;
PFNGLCOVERSTROKEPATHNVPROC                              glCoverStrokePathNV;
PFNGLCOVERAGEMASKNVPROC                                 glCoverageMaskNV;
PFNGLCOVERAGEMODULATIONNVPROC                           glCoverageModulationNV;
PFNGLCOVERAGEMODULATIONTABLENVPROC                      glCoverageModulationTableNV;
PFNGLCOVERAGEOPERATIONNVPROC                            glCoverageOperationNV;
PFNGLCREATEPERFQUERYINTELPROC                           glCreatePerfQueryINTEL;
PFNGLCREATESHADERPROGRAMVEXTPROC                        glCreateShaderProgramvEXT;
PFNGLDEBUGMESSAGECALLBACKKHRPROC                        glDebugMessageCallbackKHR;
PFNGLDEBUGMESSAGECONTROLKHRPROC                         glDebugMessageControlKHR;
PFNGLDEBUGMESSAGEINSERTKHRPROC                          glDebugMessageInsertKHR;
PFNGLDELETEFENCESNVPROC                                 glDeleteFencesNV;
PFNGLDELETEPATHSNVPROC                                  glDeletePathsNV;
PFNGLDELETEPERFMONITORSAMDPROC                          glDeletePerfMonitorsAMD;
PFNGLDELETEPERFQUERYINTELPROC                           glDeletePerfQueryINTEL;
PFNGLDELETEPROGRAMPIPELINESEXTPROC                      glDeleteProgramPipelinesEXT;
PFNGLDELETEQUERIESEXTPROC                               glDeleteQueriesEXT;
PFNGLDELETESYNCAPPLEPROC                                glDeleteSyncAPPLE;
PFNGLDELETEVERTEXARRAYSOESPROC                          glDeleteVertexArraysOES;
PFNGLDEPTHRANGEARRAYFVNVPROC                            glDepthRangeArrayfvNV;
PFNGLDEPTHRANGEINDEXEDFNVPROC                           glDepthRangeIndexedfNV;
PFNGLDISABLEDRIVERCONTROLQCOMPROC                       glDisableDriverControlQCOM;
PFNGLDISABLEIEXTPROC                                    glDisableiEXT;
PFNGLDISABLEINVPROC                                     glDisableiNV;
PFNGLDISABLEIOESPROC                                    glDisableiOES;
PFNGLDISCARDFRAMEBUFFEREXTPROC                          glDiscardFramebufferEXT;
PFNGLDRAWARRAYSINSTANCEDANGLEPROC                       glDrawArraysInstancedANGLE;
PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEEXTPROC             glDrawArraysInstancedBaseInstanceEXT;
PFNGLDRAWARRAYSINSTANCEDEXTPROC                         glDrawArraysInstancedEXT;
PFNGLDRAWARRAYSINSTANCEDNVPROC                          glDrawArraysInstancedNV;
PFNGLDRAWBUFFERSEXTPROC                                 glDrawBuffersEXT;
PFNGLDRAWBUFFERSINDEXEDEXTPROC                          glDrawBuffersIndexedEXT;
PFNGLDRAWBUFFERSNVPROC                                  glDrawBuffersNV;
PFNGLDRAWELEMENTSBASEVERTEXEXTPROC                      glDrawElementsBaseVertexEXT;
PFNGLDRAWELEMENTSBASEVERTEXOESPROC                      glDrawElementsBaseVertexOES;
PFNGLDRAWELEMENTSINSTANCEDANGLEPROC                     glDrawElementsInstancedANGLE;
PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEEXTPROC           glDrawElementsInstancedBaseInstanceEXT;
PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEEXTPROC glDrawElementsInstancedBaseVertexBaseInstanceEXT;
PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXEXTPROC             glDrawElementsInstancedBaseVertexEXT;
PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXOESPROC             glDrawElementsInstancedBaseVertexOES;
PFNGLDRAWELEMENTSINSTANCEDEXTPROC                       glDrawElementsInstancedEXT;
PFNGLDRAWELEMENTSINSTANCEDNVPROC                        glDrawElementsInstancedNV;
PFNGLDRAWRANGEELEMENTSBASEVERTEXEXTPROC                 glDrawRangeElementsBaseVertexEXT;
PFNGLDRAWRANGEELEMENTSBASEVERTEXOESPROC                 glDrawRangeElementsBaseVertexOES;
PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC           glEGLImageTargetRenderbufferStorageOES;
PFNGLEGLIMAGETARGETTEXTURE2DOESPROC                     glEGLImageTargetTexture2DOES;
PFNGLENABLEDRIVERCONTROLQCOMPROC                        glEnableDriverControlQCOM;
PFNGLENABLEIEXTPROC                                     glEnableiEXT;
PFNGLENABLEINVPROC                                      glEnableiNV;
PFNGLENABLEIOESPROC                                     glEnableiOES;
PFNGLENDCONDITIONALRENDERNVPROC                         glEndConditionalRenderNV;
PFNGLENDPERFMONITORAMDPROC                              glEndPerfMonitorAMD;
PFNGLENDPERFQUERYINTELPROC                              glEndPerfQueryINTEL;
PFNGLENDQUERYEXTPROC                                    glEndQueryEXT;
PFNGLENDTILINGQCOMPROC                                  glEndTilingQCOM;
PFNGLEXTGETBUFFERPOINTERVQCOMPROC                       glExtGetBufferPointervQCOM;
PFNGLEXTGETBUFFERSQCOMPROC                              glExtGetBuffersQCOM;
PFNGLEXTGETFRAMEBUFFERSQCOMPROC                         glExtGetFramebuffersQCOM;
PFNGLEXTGETPROGRAMBINARYSOURCEQCOMPROC                  glExtGetProgramBinarySourceQCOM;
PFNGLEXTGETPROGRAMSQCOMPROC                             glExtGetProgramsQCOM;
PFNGLEXTGETRENDERBUFFERSQCOMPROC                        glExtGetRenderbuffersQCOM;
PFNGLEXTGETSHADERSQCOMPROC                              glExtGetShadersQCOM;
PFNGLEXTGETTEXLEVELPARAMETERIVQCOMPROC                  glExtGetTexLevelParameterivQCOM;
PFNGLEXTGETTEXSUBIMAGEQCOMPROC                          glExtGetTexSubImageQCOM;
PFNGLEXTGETTEXTURESQCOMPROC                             glExtGetTexturesQCOM;
PFNGLEXTISPROGRAMBINARYQCOMPROC                         glExtIsProgramBinaryQCOM;
PFNGLEXTTEXOBJECTSTATEOVERRIDEIQCOMPROC                 glExtTexObjectStateOverrideiQCOM;
PFNGLFENCESYNCAPPLEPROC                                 glFenceSyncAPPLE;
PFNGLFINISHFENCENVPROC                                  glFinishFenceNV;
PFNGLFLUSHMAPPEDBUFFERRANGEEXTPROC                      glFlushMappedBufferRangeEXT;
PFNGLFRAGMENTCOVERAGECOLORNVPROC                        glFragmentCoverageColorNV;
PFNGLFRAMEBUFFERPIXELLOCALSTORAGESIZEEXTPROC            glFramebufferPixelLocalStorageSizeEXT;
PFNGLFRAMEBUFFERSAMPLELOCATIONSFVNVPROC                 glFramebufferSampleLocationsfvNV;
PFNGLFRAMEBUFFERTEXTURE2DDOWNSAMPLEIMGPROC              glFramebufferTexture2DDownsampleIMG;
PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC             glFramebufferTexture2DMultisampleEXT;
PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMGPROC             glFramebufferTexture2DMultisampleIMG;
PFNGLFRAMEBUFFERTEXTURE3DOESPROC                        glFramebufferTexture3DOES;
PFNGLFRAMEBUFFERTEXTUREEXTPROC                          glFramebufferTextureEXT;
PFNGLFRAMEBUFFERTEXTURELAYERDOWNSAMPLEIMGPROC           glFramebufferTextureLayerDownsampleIMG;
PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC      glFramebufferTextureMultisampleMultiviewOVR;
PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC                 glFramebufferTextureMultiviewOVR;
PFNGLFRAMEBUFFERTEXTUREOESPROC                          glFramebufferTextureOES;
PFNGLGENFENCESNVPROC                                    glGenFencesNV;
PFNGLGENPATHSNVPROC                                     glGenPathsNV;
PFNGLGENPERFMONITORSAMDPROC                             glGenPerfMonitorsAMD;
PFNGLGENPROGRAMPIPELINESEXTPROC                         glGenProgramPipelinesEXT;
PFNGLGENQUERIESEXTPROC                                  glGenQueriesEXT;
PFNGLGENVERTEXARRAYSOESPROC                             glGenVertexArraysOES;
PFNGLGETBUFFERPOINTERVOESPROC                           glGetBufferPointervOES;
PFNGLGETCOVERAGEMODULATIONTABLENVPROC                   glGetCoverageModulationTableNV;
PFNGLGETDEBUGMESSAGELOGKHRPROC                          glGetDebugMessageLogKHR;
PFNGLGETDRIVERCONTROLSTRINGQCOMPROC                     glGetDriverControlStringQCOM;
PFNGLGETDRIVERCONTROLSQCOMPROC                          glGetDriverControlsQCOM;
PFNGLGETFENCEIVNVPROC                                   glGetFenceivNV;
PFNGLGETFIRSTPERFQUERYIDINTELPROC                       glGetFirstPerfQueryIdINTEL;
PFNGLGETFLOATI_VNVPROC                                  glGetFloati_vNV;
PFNGLGETFRAGDATAINDEXEXTPROC                            glGetFragDataIndexEXT;
PFNGLGETFRAMEBUFFERPIXELLOCALSTORAGESIZEEXTPROC         glGetFramebufferPixelLocalStorageSizeEXT;
PFNGLGETGRAPHICSRESETSTATUSEXTPROC                      glGetGraphicsResetStatusEXT;
PFNGLGETGRAPHICSRESETSTATUSKHRPROC                      glGetGraphicsResetStatusKHR;
PFNGLGETIMAGEHANDLENVPROC                               glGetImageHandleNV;
PFNGLGETINTEGER64VAPPLEPROC                             glGetInteger64vAPPLE;
PFNGLGETINTEGERI_VEXTPROC                               glGetIntegeri_vEXT;
PFNGLGETINTERNALFORMATSAMPLEIVNVPROC                    glGetInternalformatSampleivNV;
PFNGLGETNEXTPERFQUERYIDINTELPROC                        glGetNextPerfQueryIdINTEL;
PFNGLGETOBJECTLABELEXTPROC                              glGetObjectLabelEXT;
PFNGLGETOBJECTLABELKHRPROC                              glGetObjectLabelKHR;
PFNGLGETOBJECTPTRLABELKHRPROC                           glGetObjectPtrLabelKHR;
PFNGLGETPATHCOMMANDSNVPROC                              glGetPathCommandsNV;
PFNGLGETPATHCOORDSNVPROC                                glGetPathCoordsNV;
PFNGLGETPATHDASHARRAYNVPROC                             glGetPathDashArrayNV;
PFNGLGETPATHLENGTHNVPROC                                glGetPathLengthNV;
PFNGLGETPATHMETRICRANGENVPROC                           glGetPathMetricRangeNV;
PFNGLGETPATHMETRICSNVPROC                               glGetPathMetricsNV;
PFNGLGETPATHPARAMETERFVNVPROC                           glGetPathParameterfvNV;
PFNGLGETPATHPARAMETERIVNVPROC                           glGetPathParameterivNV;
PFNGLGETPATHSPACINGNVPROC                               glGetPathSpacingNV;
PFNGLGETPERFCOUNTERINFOINTELPROC                        glGetPerfCounterInfoINTEL;
PFNGLGETPERFMONITORCOUNTERDATAAMDPROC                   glGetPerfMonitorCounterDataAMD;
PFNGLGETPERFMONITORCOUNTERINFOAMDPROC                   glGetPerfMonitorCounterInfoAMD;
PFNGLGETPERFMONITORCOUNTERSTRINGAMDPROC                 glGetPerfMonitorCounterStringAMD;
PFNGLGETPERFMONITORCOUNTERSAMDPROC                      glGetPerfMonitorCountersAMD;
PFNGLGETPERFMONITORGROUPSTRINGAMDPROC                   glGetPerfMonitorGroupStringAMD;
PFNGLGETPERFMONITORGROUPSAMDPROC                        glGetPerfMonitorGroupsAMD;
PFNGLGETPERFQUERYDATAINTELPROC                          glGetPerfQueryDataINTEL;
PFNGLGETPERFQUERYIDBYNAMEINTELPROC                      glGetPerfQueryIdByNameINTEL;
PFNGLGETPERFQUERYINFOINTELPROC                          glGetPerfQueryInfoINTEL;
PFNGLGETPOINTERVKHRPROC                                 glGetPointervKHR;
PFNGLGETPROGRAMBINARYOESPROC                            glGetProgramBinaryOES;
PFNGLGETPROGRAMPIPELINEINFOLOGEXTPROC                   glGetProgramPipelineInfoLogEXT;
PFNGLGETPROGRAMPIPELINEIVEXTPROC                        glGetProgramPipelineivEXT;
PFNGLGETPROGRAMRESOURCELOCATIONINDEXEXTPROC             glGetProgramResourceLocationIndexEXT;
PFNGLGETPROGRAMRESOURCEFVNVPROC                         glGetProgramResourcefvNV;
PFNGLGETQUERYOBJECTI64VEXTPROC                          glGetQueryObjecti64vEXT;
PFNGLGETQUERYOBJECTIVEXTPROC                            glGetQueryObjectivEXT;
PFNGLGETQUERYOBJECTUI64VEXTPROC                         glGetQueryObjectui64vEXT;
PFNGLGETQUERYOBJECTUIVEXTPROC                           glGetQueryObjectuivEXT;
PFNGLGETQUERYIVEXTPROC                                  glGetQueryivEXT;
PFNGLGETSAMPLERPARAMETERIIVEXTPROC                      glGetSamplerParameterIivEXT;
PFNGLGETSAMPLERPARAMETERIIVOESPROC                      glGetSamplerParameterIivOES;
PFNGLGETSAMPLERPARAMETERIUIVEXTPROC                     glGetSamplerParameterIuivEXT;
PFNGLGETSAMPLERPARAMETERIUIVOESPROC                     glGetSamplerParameterIuivOES;
PFNGLGETSYNCIVAPPLEPROC                                 glGetSyncivAPPLE;
PFNGLGETTEXPARAMETERIIVEXTPROC                          glGetTexParameterIivEXT;
PFNGLGETTEXPARAMETERIIVOESPROC                          glGetTexParameterIivOES;
PFNGLGETTEXPARAMETERIUIVEXTPROC                         glGetTexParameterIuivEXT;
PFNGLGETTEXPARAMETERIUIVOESPROC                         glGetTexParameterIuivOES;
PFNGLGETTEXTUREHANDLENVPROC                             glGetTextureHandleNV;
PFNGLGETTEXTURESAMPLERHANDLENVPROC                      glGetTextureSamplerHandleNV;
PFNGLGETTRANSLATEDSHADERSOURCEANGLEPROC                 glGetTranslatedShaderSourceANGLE;
PFNGLGETNUNIFORMFVEXTPROC                               glGetnUniformfvEXT;
PFNGLGETNUNIFORMFVKHRPROC                               glGetnUniformfvKHR;
PFNGLGETNUNIFORMIVEXTPROC                               glGetnUniformivEXT;
PFNGLGETNUNIFORMIVKHRPROC                               glGetnUniformivKHR;
PFNGLGETNUNIFORMUIVKHRPROC                              glGetnUniformuivKHR;
PFNGLINSERTEVENTMARKEREXTPROC                           glInsertEventMarkerEXT;
PFNGLINTERPOLATEPATHSNVPROC                             glInterpolatePathsNV;
PFNGLISENABLEDIEXTPROC                                  glIsEnablediEXT;
PFNGLISENABLEDINVPROC                                   glIsEnablediNV;
PFNGLISENABLEDIOESPROC                                  glIsEnablediOES;
PFNGLISFENCENVPROC                                      glIsFenceNV;
PFNGLISIMAGEHANDLERESIDENTNVPROC                        glIsImageHandleResidentNV;
PFNGLISPATHNVPROC                                       glIsPathNV;
PFNGLISPOINTINFILLPATHNVPROC                            glIsPointInFillPathNV;
PFNGLISPOINTINSTROKEPATHNVPROC                          glIsPointInStrokePathNV;
PFNGLISPROGRAMPIPELINEEXTPROC                           glIsProgramPipelineEXT;
PFNGLISQUERYEXTPROC                                     glIsQueryEXT;
PFNGLISSYNCAPPLEPROC                                    glIsSyncAPPLE;
PFNGLISTEXTUREHANDLERESIDENTNVPROC                      glIsTextureHandleResidentNV;
PFNGLISVERTEXARRAYOESPROC                               glIsVertexArrayOES;
PFNGLLABELOBJECTEXTPROC                                 glLabelObjectEXT;
PFNGLMAKEIMAGEHANDLENONRESIDENTNVPROC                   glMakeImageHandleNonResidentNV;
PFNGLMAKEIMAGEHANDLERESIDENTNVPROC                      glMakeImageHandleResidentNV;
PFNGLMAKETEXTUREHANDLENONRESIDENTNVPROC                 glMakeTextureHandleNonResidentNV;
PFNGLMAKETEXTUREHANDLERESIDENTNVPROC                    glMakeTextureHandleResidentNV;
PFNGLMAPBUFFEROESPROC                                   glMapBufferOES;
PFNGLMAPBUFFERRANGEEXTPROC                              glMapBufferRangeEXT;
PFNGLMATRIXLOAD3X2FNVPROC                               glMatrixLoad3x2fNV;
PFNGLMATRIXLOAD3X3FNVPROC                               glMatrixLoad3x3fNV;
PFNGLMATRIXLOADTRANSPOSE3X3FNVPROC                      glMatrixLoadTranspose3x3fNV;
PFNGLMATRIXMULT3X2FNVPROC                               glMatrixMult3x2fNV;
PFNGLMATRIXMULT3X3FNVPROC                               glMatrixMult3x3fNV;
PFNGLMATRIXMULTTRANSPOSE3X3FNVPROC                      glMatrixMultTranspose3x3fNV;
PFNGLMINSAMPLESHADINGOESPROC                            glMinSampleShadingOES;
PFNGLMULTIDRAWARRAYSEXTPROC                             glMultiDrawArraysEXT;
PFNGLMULTIDRAWARRAYSINDIRECTEXTPROC                     glMultiDrawArraysIndirectEXT;
PFNGLMULTIDRAWELEMENTSBASEVERTEXEXTPROC                 glMultiDrawElementsBaseVertexEXT;
PFNGLMULTIDRAWELEMENTSBASEVERTEXOESPROC                 glMultiDrawElementsBaseVertexOES;
PFNGLMULTIDRAWELEMENTSEXTPROC                           glMultiDrawElementsEXT;
PFNGLMULTIDRAWELEMENTSINDIRECTEXTPROC                   glMultiDrawElementsIndirectEXT;
PFNGLNAMEDFRAMEBUFFERSAMPLELOCATIONSFVNVPROC            glNamedFramebufferSampleLocationsfvNV;
PFNGLOBJECTLABELKHRPROC                                 glObjectLabelKHR;
PFNGLOBJECTPTRLABELKHRPROC                              glObjectPtrLabelKHR;
PFNGLPATCHPARAMETERIEXTPROC                             glPatchParameteriEXT;
PFNGLPATCHPARAMETERIOESPROC                             glPatchParameteriOES;
PFNGLPATHCOMMANDSNVPROC                                 glPathCommandsNV;
PFNGLPATHCOORDSNVPROC                                   glPathCoordsNV;
PFNGLPATHCOVERDEPTHFUNCNVPROC                           glPathCoverDepthFuncNV;
PFNGLPATHDASHARRAYNVPROC                                glPathDashArrayNV;
PFNGLPATHGLYPHINDEXARRAYNVPROC                          glPathGlyphIndexArrayNV;
PFNGLPATHGLYPHINDEXRANGENVPROC                          glPathGlyphIndexRangeNV;
PFNGLPATHGLYPHRANGENVPROC                               glPathGlyphRangeNV;
PFNGLPATHGLYPHSNVPROC                                   glPathGlyphsNV;
PFNGLPATHMEMORYGLYPHINDEXARRAYNVPROC                    glPathMemoryGlyphIndexArrayNV;
PFNGLPATHPARAMETERFNVPROC                               glPathParameterfNV;
PFNGLPATHPARAMETERFVNVPROC                              glPathParameterfvNV;
PFNGLPATHPARAMETERINVPROC                               glPathParameteriNV;
PFNGLPATHPARAMETERIVNVPROC                              glPathParameterivNV;
PFNGLPATHSTENCILDEPTHOFFSETNVPROC                       glPathStencilDepthOffsetNV;
PFNGLPATHSTENCILFUNCNVPROC                              glPathStencilFuncNV;
PFNGLPATHSTRINGNVPROC                                   glPathStringNV;
PFNGLPATHSUBCOMMANDSNVPROC                              glPathSubCommandsNV;
PFNGLPATHSUBCOORDSNVPROC                                glPathSubCoordsNV;
PFNGLPOINTALONGPATHNVPROC                               glPointAlongPathNV;
PFNGLPOLYGONMODENVPROC                                  glPolygonModeNV;
PFNGLPOLYGONOFFSETCLAMPEXTPROC                          glPolygonOffsetClampEXT;
PFNGLPOPDEBUGGROUPKHRPROC                               glPopDebugGroupKHR;
PFNGLPOPGROUPMARKEREXTPROC                              glPopGroupMarkerEXT;
PFNGLPRIMITIVEBOUNDINGBOXEXTPROC                        glPrimitiveBoundingBoxEXT;
PFNGLPRIMITIVEBOUNDINGBOXOESPROC                        glPrimitiveBoundingBoxOES;
PFNGLPROGRAMBINARYOESPROC                               glProgramBinaryOES;
PFNGLPROGRAMPARAMETERIEXTPROC                           glProgramParameteriEXT;
PFNGLPROGRAMPATHFRAGMENTINPUTGENNVPROC                  glProgramPathFragmentInputGenNV;
PFNGLPROGRAMUNIFORM1FEXTPROC                            glProgramUniform1fEXT;
PFNGLPROGRAMUNIFORM1FVEXTPROC                           glProgramUniform1fvEXT;
PFNGLPROGRAMUNIFORM1IEXTPROC                            glProgramUniform1iEXT;
PFNGLPROGRAMUNIFORM1IVEXTPROC                           glProgramUniform1ivEXT;
PFNGLPROGRAMUNIFORM1UIEXTPROC                           glProgramUniform1uiEXT;
PFNGLPROGRAMUNIFORM1UIVEXTPROC                          glProgramUniform1uivEXT;
PFNGLPROGRAMUNIFORM2FEXTPROC                            glProgramUniform2fEXT;
PFNGLPROGRAMUNIFORM2FVEXTPROC                           glProgramUniform2fvEXT;
PFNGLPROGRAMUNIFORM2IEXTPROC                            glProgramUniform2iEXT;
PFNGLPROGRAMUNIFORM2IVEXTPROC                           glProgramUniform2ivEXT;
PFNGLPROGRAMUNIFORM2UIEXTPROC                           glProgramUniform2uiEXT;
PFNGLPROGRAMUNIFORM2UIVEXTPROC                          glProgramUniform2uivEXT;
PFNGLPROGRAMUNIFORM3FEXTPROC                            glProgramUniform3fEXT;
PFNGLPROGRAMUNIFORM3FVEXTPROC                           glProgramUniform3fvEXT;
PFNGLPROGRAMUNIFORM3IEXTPROC                            glProgramUniform3iEXT;
PFNGLPROGRAMUNIFORM3IVEXTPROC                           glProgramUniform3ivEXT;
PFNGLPROGRAMUNIFORM3UIEXTPROC                           glProgramUniform3uiEXT;
PFNGLPROGRAMUNIFORM3UIVEXTPROC                          glProgramUniform3uivEXT;
PFNGLPROGRAMUNIFORM4FEXTPROC                            glProgramUniform4fEXT;
PFNGLPROGRAMUNIFORM4FVEXTPROC                           glProgramUniform4fvEXT;
PFNGLPROGRAMUNIFORM4IEXTPROC                            glProgramUniform4iEXT;
PFNGLPROGRAMUNIFORM4IVEXTPROC                           glProgramUniform4ivEXT;
PFNGLPROGRAMUNIFORM4UIEXTPROC                           glProgramUniform4uiEXT;
PFNGLPROGRAMUNIFORM4UIVEXTPROC                          glProgramUniform4uivEXT;
PFNGLPROGRAMUNIFORMHANDLEUI64NVPROC                     glProgramUniformHandleui64NV;
PFNGLPROGRAMUNIFORMHANDLEUI64VNVPROC                    glProgramUniformHandleui64vNV;
PFNGLPROGRAMUNIFORMMATRIX2FVEXTPROC                     glProgramUniformMatrix2fvEXT;
PFNGLPROGRAMUNIFORMMATRIX2X3FVEXTPROC                   glProgramUniformMatrix2x3fvEXT;
PFNGLPROGRAMUNIFORMMATRIX2X4FVEXTPROC                   glProgramUniformMatrix2x4fvEXT;
PFNGLPROGRAMUNIFORMMATRIX3FVEXTPROC                     glProgramUniformMatrix3fvEXT;
PFNGLPROGRAMUNIFORMMATRIX3X2FVEXTPROC                   glProgramUniformMatrix3x2fvEXT;
PFNGLPROGRAMUNIFORMMATRIX3X4FVEXTPROC                   glProgramUniformMatrix3x4fvEXT;
PFNGLPROGRAMUNIFORMMATRIX4FVEXTPROC                     glProgramUniformMatrix4fvEXT;
PFNGLPROGRAMUNIFORMMATRIX4X2FVEXTPROC                   glProgramUniformMatrix4x2fvEXT;
PFNGLPROGRAMUNIFORMMATRIX4X3FVEXTPROC                   glProgramUniformMatrix4x3fvEXT;
PFNGLPUSHDEBUGGROUPKHRPROC                              glPushDebugGroupKHR;
PFNGLPUSHGROUPMARKEREXTPROC                             glPushGroupMarkerEXT;
PFNGLQUERYCOUNTEREXTPROC                                glQueryCounterEXT;
PFNGLRASTERSAMPLESEXTPROC                               glRasterSamplesEXT;
PFNGLREADBUFFERINDEXEDEXTPROC                           glReadBufferIndexedEXT;
PFNGLREADBUFFERNVPROC                                   glReadBufferNV;
PFNGLREADNPIXELSEXTPROC                                 glReadnPixelsEXT;
PFNGLREADNPIXELSKHRPROC                                 glReadnPixelsKHR;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEANGLEPROC            glRenderbufferStorageMultisampleANGLE;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEAPPLEPROC            glRenderbufferStorageMultisampleAPPLE;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC              glRenderbufferStorageMultisampleEXT;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMGPROC              glRenderbufferStorageMultisampleIMG;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLENVPROC               glRenderbufferStorageMultisampleNV;
PFNGLRESOLVEDEPTHVALUESNVPROC                           glResolveDepthValuesNV;
PFNGLRESOLVEMULTISAMPLEFRAMEBUFFERAPPLEPROC             glResolveMultisampleFramebufferAPPLE;
PFNGLSAMPLERPARAMETERIIVEXTPROC                         glSamplerParameterIivEXT;
PFNGLSAMPLERPARAMETERIIVOESPROC                         glSamplerParameterIivOES;
PFNGLSAMPLERPARAMETERIUIVEXTPROC                        glSamplerParameterIuivEXT;
PFNGLSAMPLERPARAMETERIUIVOESPROC                        glSamplerParameterIuivOES;
PFNGLSCISSORARRAYVNVPROC                                glScissorArrayvNV;
PFNGLSCISSORINDEXEDNVPROC                               glScissorIndexedNV;
PFNGLSCISSORINDEXEDVNVPROC                              glScissorIndexedvNV;
PFNGLSELECTPERFMONITORCOUNTERSAMDPROC                   glSelectPerfMonitorCountersAMD;
PFNGLSETFENCENVPROC                                     glSetFenceNV;
PFNGLSTARTTILINGQCOMPROC                                glStartTilingQCOM;
PFNGLSTENCILFILLPATHINSTANCEDNVPROC                     glStencilFillPathInstancedNV;
PFNGLSTENCILFILLPATHNVPROC                              glStencilFillPathNV;
PFNGLSTENCILSTROKEPATHINSTANCEDNVPROC                   glStencilStrokePathInstancedNV;
PFNGLSTENCILSTROKEPATHNVPROC                            glStencilStrokePathNV;
PFNGLSTENCILTHENCOVERFILLPATHINSTANCEDNVPROC            glStencilThenCoverFillPathInstancedNV;
PFNGLSTENCILTHENCOVERFILLPATHNVPROC                     glStencilThenCoverFillPathNV;
PFNGLSTENCILTHENCOVERSTROKEPATHINSTANCEDNVPROC          glStencilThenCoverStrokePathInstancedNV;
PFNGLSTENCILTHENCOVERSTROKEPATHNVPROC                   glStencilThenCoverStrokePathNV;
PFNGLSUBPIXELPRECISIONBIASNVPROC                        glSubpixelPrecisionBiasNV;
PFNGLTESTFENCENVPROC                                    glTestFenceNV;
PFNGLTEXBUFFEREXTPROC                                   glTexBufferEXT;
PFNGLTEXBUFFEROESPROC                                   glTexBufferOES;
PFNGLTEXBUFFERRANGEEXTPROC                              glTexBufferRangeEXT;
PFNGLTEXBUFFERRANGEOESPROC                              glTexBufferRangeOES;
PFNGLTEXIMAGE3DOESPROC                                  glTexImage3DOES;
PFNGLTEXPAGECOMMITMENTEXTPROC                           glTexPageCommitmentEXT;
PFNGLTEXPARAMETERIIVEXTPROC                             glTexParameterIivEXT;
PFNGLTEXPARAMETERIIVOESPROC                             glTexParameterIivOES;
PFNGLTEXPARAMETERIUIVEXTPROC                            glTexParameterIuivEXT;
PFNGLTEXPARAMETERIUIVOESPROC                            glTexParameterIuivOES;
PFNGLTEXSTORAGE1DEXTPROC                                glTexStorage1DEXT;
PFNGLTEXSTORAGE2DEXTPROC                                glTexStorage2DEXT;
PFNGLTEXSTORAGE3DEXTPROC                                glTexStorage3DEXT;
PFNGLTEXSTORAGE3DMULTISAMPLEOESPROC                     glTexStorage3DMultisampleOES;
PFNGLTEXSUBIMAGE3DOESPROC                               glTexSubImage3DOES;
PFNGLTEXTURESTORAGE1DEXTPROC                            glTextureStorage1DEXT;
PFNGLTEXTURESTORAGE2DEXTPROC                            glTextureStorage2DEXT;
PFNGLTEXTURESTORAGE3DEXTPROC                            glTextureStorage3DEXT;
PFNGLTEXTUREVIEWEXTPROC                                 glTextureViewEXT;
PFNGLTEXTUREVIEWOESPROC                                 glTextureViewOES;
PFNGLTRANSFORMPATHNVPROC                                glTransformPathNV;
PFNGLUNIFORMHANDLEUI64NVPROC                            glUniformHandleui64NV;
PFNGLUNIFORMHANDLEUI64VNVPROC                           glUniformHandleui64vNV;
PFNGLUNIFORMMATRIX2X3FVNVPROC                           glUniformMatrix2x3fvNV;
PFNGLUNIFORMMATRIX2X4FVNVPROC                           glUniformMatrix2x4fvNV;
PFNGLUNIFORMMATRIX3X2FVNVPROC                           glUniformMatrix3x2fvNV;
PFNGLUNIFORMMATRIX3X4FVNVPROC                           glUniformMatrix3x4fvNV;
PFNGLUNIFORMMATRIX4X2FVNVPROC                           glUniformMatrix4x2fvNV;
PFNGLUNIFORMMATRIX4X3FVNVPROC                           glUniformMatrix4x3fvNV;
PFNGLUNMAPBUFFEROESPROC                                 glUnmapBufferOES;
PFNGLUSEPROGRAMSTAGESEXTPROC                            glUseProgramStagesEXT;
PFNGLVALIDATEPROGRAMPIPELINEEXTPROC                     glValidateProgramPipelineEXT;
PFNGLVERTEXATTRIBDIVISORANGLEPROC                       glVertexAttribDivisorANGLE;
PFNGLVERTEXATTRIBDIVISOREXTPROC                         glVertexAttribDivisorEXT;
PFNGLVERTEXATTRIBDIVISORNVPROC                          glVertexAttribDivisorNV;
PFNGLVIEWPORTARRAYVNVPROC                               glViewportArrayvNV;
PFNGLVIEWPORTINDEXEDFNVPROC                             glViewportIndexedfNV;
PFNGLVIEWPORTINDEXEDFVNVPROC                            glViewportIndexedfvNV;
PFNGLWAITSYNCAPPLEPROC                                  glWaitSyncAPPLE;
PFNGLWEIGHTPATHSNVPROC                                  glWeightPathsNV;

static void load_procs(glGetProcAddressProc proc)
{
    glActiveShaderProgramEXT = (PFNGLACTIVESHADERPROGRAMEXTPROC)proc("glActiveShaderProgramEXT");
    glAlphaFuncQCOM = (PFNGLALPHAFUNCQCOMPROC)proc("glAlphaFuncQCOM");
    glApplyFramebufferAttachmentCMAAINTEL = (PFNGLAPPLYFRAMEBUFFERATTACHMENTCMAAINTELPROC)proc("glApplyFramebufferAttachmentCMAAINTEL");
    glBeginConditionalRenderNV = (PFNGLBEGINCONDITIONALRENDERNVPROC)proc("glBeginConditionalRenderNV");
    glBeginPerfMonitorAMD = (PFNGLBEGINPERFMONITORAMDPROC)proc("glBeginPerfMonitorAMD");
    glBeginPerfQueryINTEL = (PFNGLBEGINPERFQUERYINTELPROC)proc("glBeginPerfQueryINTEL");
    glBeginQueryEXT = (PFNGLBEGINQUERYEXTPROC)proc("glBeginQueryEXT");
    glBindFragDataLocationEXT = (PFNGLBINDFRAGDATALOCATIONEXTPROC)proc("glBindFragDataLocationEXT");
    glBindFragDataLocationIndexedEXT = (PFNGLBINDFRAGDATALOCATIONINDEXEDEXTPROC)proc("glBindFragDataLocationIndexedEXT");
    glBindProgramPipelineEXT = (PFNGLBINDPROGRAMPIPELINEEXTPROC)proc("glBindProgramPipelineEXT");
    glBindVertexArrayOES = (PFNGLBINDVERTEXARRAYOESPROC)proc("glBindVertexArrayOES");
    glBlendBarrierKHR = (PFNGLBLENDBARRIERKHRPROC)proc("glBlendBarrierKHR");
    glBlendBarrierNV = (PFNGLBLENDBARRIERNVPROC)proc("glBlendBarrierNV");
    glBlendEquationSeparateiEXT = (PFNGLBLENDEQUATIONSEPARATEIEXTPROC)proc("glBlendEquationSeparateiEXT");
    glBlendEquationSeparateiOES = (PFNGLBLENDEQUATIONSEPARATEIOESPROC)proc("glBlendEquationSeparateiOES");
    glBlendEquationiEXT = (PFNGLBLENDEQUATIONIEXTPROC)proc("glBlendEquationiEXT");
    glBlendEquationiOES = (PFNGLBLENDEQUATIONIOESPROC)proc("glBlendEquationiOES");
    glBlendFuncSeparateiEXT = (PFNGLBLENDFUNCSEPARATEIEXTPROC)proc("glBlendFuncSeparateiEXT");
    glBlendFuncSeparateiOES = (PFNGLBLENDFUNCSEPARATEIOESPROC)proc("glBlendFuncSeparateiOES");
    glBlendFunciEXT = (PFNGLBLENDFUNCIEXTPROC)proc("glBlendFunciEXT");
    glBlendFunciOES = (PFNGLBLENDFUNCIOESPROC)proc("glBlendFunciOES");
    glBlendParameteriNV = (PFNGLBLENDPARAMETERINVPROC)proc("glBlendParameteriNV");
    glBlitFramebufferANGLE = (PFNGLBLITFRAMEBUFFERANGLEPROC)proc("glBlitFramebufferANGLE");
    glBlitFramebufferNV = (PFNGLBLITFRAMEBUFFERNVPROC)proc("glBlitFramebufferNV");
    glBufferStorageEXT = (PFNGLBUFFERSTORAGEEXTPROC)proc("glBufferStorageEXT");
    glClearPixelLocalStorageuiEXT = (PFNGLCLEARPIXELLOCALSTORAGEUIEXTPROC)proc("glClearPixelLocalStorageuiEXT");
    glClientWaitSyncAPPLE = (PFNGLCLIENTWAITSYNCAPPLEPROC)proc("glClientWaitSyncAPPLE");
    glColorMaskiEXT = (PFNGLCOLORMASKIEXTPROC)proc("glColorMaskiEXT");
    glColorMaskiOES = (PFNGLCOLORMASKIOESPROC)proc("glColorMaskiOES");
    glCompressedTexImage3DOES = (PFNGLCOMPRESSEDTEXIMAGE3DOESPROC)proc("glCompressedTexImage3DOES");
    glCompressedTexSubImage3DOES = (PFNGLCOMPRESSEDTEXSUBIMAGE3DOESPROC)proc("glCompressedTexSubImage3DOES");
    glCopyBufferSubDataNV = (PFNGLCOPYBUFFERSUBDATANVPROC)proc("glCopyBufferSubDataNV");
    glCopyImageSubDataEXT = (PFNGLCOPYIMAGESUBDATAEXTPROC)proc("glCopyImageSubDataEXT");
    glCopyImageSubDataOES = (PFNGLCOPYIMAGESUBDATAOESPROC)proc("glCopyImageSubDataOES");
    glCopyPathNV = (PFNGLCOPYPATHNVPROC)proc("glCopyPathNV");
    glCopyTexSubImage3DOES = (PFNGLCOPYTEXSUBIMAGE3DOESPROC)proc("glCopyTexSubImage3DOES");
    glCopyTextureLevelsAPPLE = (PFNGLCOPYTEXTURELEVELSAPPLEPROC)proc("glCopyTextureLevelsAPPLE");
    glCoverFillPathInstancedNV = (PFNGLCOVERFILLPATHINSTANCEDNVPROC)proc("glCoverFillPathInstancedNV");
    glCoverFillPathNV = (PFNGLCOVERFILLPATHNVPROC)proc("glCoverFillPathNV");
    glCoverStrokePathInstancedNV = (PFNGLCOVERSTROKEPATHINSTANCEDNVPROC)proc("glCoverStrokePathInstancedNV");
    glCoverStrokePathNV = (PFNGLCOVERSTROKEPATHNVPROC)proc("glCoverStrokePathNV");
    glCoverageMaskNV = (PFNGLCOVERAGEMASKNVPROC)proc("glCoverageMaskNV");
    glCoverageModulationNV = (PFNGLCOVERAGEMODULATIONNVPROC)proc("glCoverageModulationNV");
    glCoverageModulationTableNV = (PFNGLCOVERAGEMODULATIONTABLENVPROC)proc("glCoverageModulationTableNV");
    glCoverageOperationNV = (PFNGLCOVERAGEOPERATIONNVPROC)proc("glCoverageOperationNV");
    glCreatePerfQueryINTEL = (PFNGLCREATEPERFQUERYINTELPROC)proc("glCreatePerfQueryINTEL");
    glCreateShaderProgramvEXT = (PFNGLCREATESHADERPROGRAMVEXTPROC)proc("glCreateShaderProgramvEXT");
    glDebugMessageCallbackKHR = (PFNGLDEBUGMESSAGECALLBACKKHRPROC)proc("glDebugMessageCallbackKHR");
    glDebugMessageControlKHR = (PFNGLDEBUGMESSAGECONTROLKHRPROC)proc("glDebugMessageControlKHR");
    glDebugMessageInsertKHR = (PFNGLDEBUGMESSAGEINSERTKHRPROC)proc("glDebugMessageInsertKHR");
    glDeleteFencesNV = (PFNGLDELETEFENCESNVPROC)proc("glDeleteFencesNV");
    glDeletePathsNV = (PFNGLDELETEPATHSNVPROC)proc("glDeletePathsNV");
    glDeletePerfMonitorsAMD = (PFNGLDELETEPERFMONITORSAMDPROC)proc("glDeletePerfMonitorsAMD");
    glDeletePerfQueryINTEL = (PFNGLDELETEPERFQUERYINTELPROC)proc("glDeletePerfQueryINTEL");
    glDeleteProgramPipelinesEXT = (PFNGLDELETEPROGRAMPIPELINESEXTPROC)proc("glDeleteProgramPipelinesEXT");
    glDeleteQueriesEXT = (PFNGLDELETEQUERIESEXTPROC)proc("glDeleteQueriesEXT");
    glDeleteSyncAPPLE = (PFNGLDELETESYNCAPPLEPROC)proc("glDeleteSyncAPPLE");
    glDeleteVertexArraysOES = (PFNGLDELETEVERTEXARRAYSOESPROC)proc("glDeleteVertexArraysOES");
    glDepthRangeArrayfvNV = (PFNGLDEPTHRANGEARRAYFVNVPROC)proc("glDepthRangeArrayfvNV");
    glDepthRangeIndexedfNV = (PFNGLDEPTHRANGEINDEXEDFNVPROC)proc("glDepthRangeIndexedfNV");
    glDisableDriverControlQCOM = (PFNGLDISABLEDRIVERCONTROLQCOMPROC)proc("glDisableDriverControlQCOM");
    glDisableiEXT = (PFNGLDISABLEIEXTPROC)proc("glDisableiEXT");
    glDisableiNV = (PFNGLDISABLEINVPROC)proc("glDisableiNV");
    glDisableiOES = (PFNGLDISABLEIOESPROC)proc("glDisableiOES");
    glDiscardFramebufferEXT = (PFNGLDISCARDFRAMEBUFFEREXTPROC)proc("glDiscardFramebufferEXT");
    glDrawArraysInstancedANGLE = (PFNGLDRAWARRAYSINSTANCEDANGLEPROC)proc("glDrawArraysInstancedANGLE");
    glDrawArraysInstancedBaseInstanceEXT = (PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEEXTPROC)proc("glDrawArraysInstancedBaseInstanceEXT");
    glDrawArraysInstancedEXT = (PFNGLDRAWARRAYSINSTANCEDEXTPROC)proc("glDrawArraysInstancedEXT");
    glDrawArraysInstancedNV = (PFNGLDRAWARRAYSINSTANCEDNVPROC)proc("glDrawArraysInstancedNV");
    glDrawBuffersEXT = (PFNGLDRAWBUFFERSEXTPROC)proc("glDrawBuffersEXT");
    glDrawBuffersIndexedEXT = (PFNGLDRAWBUFFERSINDEXEDEXTPROC)proc("glDrawBuffersIndexedEXT");
    glDrawBuffersNV = (PFNGLDRAWBUFFERSNVPROC)proc("glDrawBuffersNV");
    glDrawElementsBaseVertexEXT = (PFNGLDRAWELEMENTSBASEVERTEXEXTPROC)proc("glDrawElementsBaseVertexEXT");
    glDrawElementsBaseVertexOES = (PFNGLDRAWELEMENTSBASEVERTEXOESPROC)proc("glDrawElementsBaseVertexOES");
    glDrawElementsInstancedANGLE = (PFNGLDRAWELEMENTSINSTANCEDANGLEPROC)proc("glDrawElementsInstancedANGLE");
    glDrawElementsInstancedBaseInstanceEXT = (PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEEXTPROC)proc("glDrawElementsInstancedBaseInstanceEXT");
    glDrawElementsInstancedBaseVertexBaseInstanceEXT = (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEEXTPROC)proc("glDrawElementsInstancedBaseVertexBaseInstanceEXT");
    glDrawElementsInstancedBaseVertexEXT = (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXEXTPROC)proc("glDrawElementsInstancedBaseVertexEXT");
    glDrawElementsInstancedBaseVertexOES = (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXOESPROC)proc("glDrawElementsInstancedBaseVertexOES");
    glDrawElementsInstancedEXT = (PFNGLDRAWELEMENTSINSTANCEDEXTPROC)proc("glDrawElementsInstancedEXT");
    glDrawElementsInstancedNV = (PFNGLDRAWELEMENTSINSTANCEDNVPROC)proc("glDrawElementsInstancedNV");
    glDrawRangeElementsBaseVertexEXT = (PFNGLDRAWRANGEELEMENTSBASEVERTEXEXTPROC)proc("glDrawRangeElementsBaseVertexEXT");
    glDrawRangeElementsBaseVertexOES = (PFNGLDRAWRANGEELEMENTSBASEVERTEXOESPROC)proc("glDrawRangeElementsBaseVertexOES");
    glEGLImageTargetRenderbufferStorageOES = (PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC)proc("glEGLImageTargetRenderbufferStorageOES");
    glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)proc("glEGLImageTargetTexture2DOES");
    glEnableDriverControlQCOM = (PFNGLENABLEDRIVERCONTROLQCOMPROC)proc("glEnableDriverControlQCOM");
    glEnableiEXT = (PFNGLENABLEIEXTPROC)proc("glEnableiEXT");
    glEnableiNV = (PFNGLENABLEINVPROC)proc("glEnableiNV");
    glEnableiOES = (PFNGLENABLEIOESPROC)proc("glEnableiOES");
    glEndConditionalRenderNV = (PFNGLENDCONDITIONALRENDERNVPROC)proc("glEndConditionalRenderNV");
    glEndPerfMonitorAMD = (PFNGLENDPERFMONITORAMDPROC)proc("glEndPerfMonitorAMD");
    glEndPerfQueryINTEL = (PFNGLENDPERFQUERYINTELPROC)proc("glEndPerfQueryINTEL");
    glEndQueryEXT = (PFNGLENDQUERYEXTPROC)proc("glEndQueryEXT");
    glEndTilingQCOM = (PFNGLENDTILINGQCOMPROC)proc("glEndTilingQCOM");
    glExtGetBufferPointervQCOM = (PFNGLEXTGETBUFFERPOINTERVQCOMPROC)proc("glExtGetBufferPointervQCOM");
    glExtGetBuffersQCOM = (PFNGLEXTGETBUFFERSQCOMPROC)proc("glExtGetBuffersQCOM");
    glExtGetFramebuffersQCOM = (PFNGLEXTGETFRAMEBUFFERSQCOMPROC)proc("glExtGetFramebuffersQCOM");
    glExtGetProgramBinarySourceQCOM = (PFNGLEXTGETPROGRAMBINARYSOURCEQCOMPROC)proc("glExtGetProgramBinarySourceQCOM");
    glExtGetProgramsQCOM = (PFNGLEXTGETPROGRAMSQCOMPROC)proc("glExtGetProgramsQCOM");
    glExtGetRenderbuffersQCOM = (PFNGLEXTGETRENDERBUFFERSQCOMPROC)proc("glExtGetRenderbuffersQCOM");
    glExtGetShadersQCOM = (PFNGLEXTGETSHADERSQCOMPROC)proc("glExtGetShadersQCOM");
    glExtGetTexLevelParameterivQCOM = (PFNGLEXTGETTEXLEVELPARAMETERIVQCOMPROC)proc("glExtGetTexLevelParameterivQCOM");
    glExtGetTexSubImageQCOM = (PFNGLEXTGETTEXSUBIMAGEQCOMPROC)proc("glExtGetTexSubImageQCOM");
    glExtGetTexturesQCOM = (PFNGLEXTGETTEXTURESQCOMPROC)proc("glExtGetTexturesQCOM");
    glExtIsProgramBinaryQCOM = (PFNGLEXTISPROGRAMBINARYQCOMPROC)proc("glExtIsProgramBinaryQCOM");
    glExtTexObjectStateOverrideiQCOM = (PFNGLEXTTEXOBJECTSTATEOVERRIDEIQCOMPROC)proc("glExtTexObjectStateOverrideiQCOM");
    glFenceSyncAPPLE = (PFNGLFENCESYNCAPPLEPROC)proc("glFenceSyncAPPLE");
    glFinishFenceNV = (PFNGLFINISHFENCENVPROC)proc("glFinishFenceNV");
    glFlushMappedBufferRangeEXT = (PFNGLFLUSHMAPPEDBUFFERRANGEEXTPROC)proc("glFlushMappedBufferRangeEXT");
    glFragmentCoverageColorNV = (PFNGLFRAGMENTCOVERAGECOLORNVPROC)proc("glFragmentCoverageColorNV");
    glFramebufferPixelLocalStorageSizeEXT = (PFNGLFRAMEBUFFERPIXELLOCALSTORAGESIZEEXTPROC)proc("glFramebufferPixelLocalStorageSizeEXT");
    glFramebufferSampleLocationsfvNV = (PFNGLFRAMEBUFFERSAMPLELOCATIONSFVNVPROC)proc("glFramebufferSampleLocationsfvNV");
    glFramebufferTexture2DDownsampleIMG = (PFNGLFRAMEBUFFERTEXTURE2DDOWNSAMPLEIMGPROC)proc("glFramebufferTexture2DDownsampleIMG");
    glFramebufferTexture2DMultisampleEXT = (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)proc("glFramebufferTexture2DMultisampleEXT");
    glFramebufferTexture2DMultisampleIMG = (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMGPROC)proc("glFramebufferTexture2DMultisampleIMG");
    glFramebufferTexture3DOES = (PFNGLFRAMEBUFFERTEXTURE3DOESPROC)proc("glFramebufferTexture3DOES");
    glFramebufferTextureEXT = (PFNGLFRAMEBUFFERTEXTUREEXTPROC)proc("glFramebufferTextureEXT");
    glFramebufferTextureLayerDownsampleIMG = (PFNGLFRAMEBUFFERTEXTURELAYERDOWNSAMPLEIMGPROC)proc("glFramebufferTextureLayerDownsampleIMG");
    glFramebufferTextureMultisampleMultiviewOVR = (PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC)proc("glFramebufferTextureMultisampleMultiviewOVR");
    glFramebufferTextureMultiviewOVR = (PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)proc("glFramebufferTextureMultiviewOVR");
    glFramebufferTextureOES = (PFNGLFRAMEBUFFERTEXTUREOESPROC)proc("glFramebufferTextureOES");
    glGenFencesNV = (PFNGLGENFENCESNVPROC)proc("glGenFencesNV");
    glGenPathsNV = (PFNGLGENPATHSNVPROC)proc("glGenPathsNV");
    glGenPerfMonitorsAMD = (PFNGLGENPERFMONITORSAMDPROC)proc("glGenPerfMonitorsAMD");
    glGenProgramPipelinesEXT = (PFNGLGENPROGRAMPIPELINESEXTPROC)proc("glGenProgramPipelinesEXT");
    glGenQueriesEXT = (PFNGLGENQUERIESEXTPROC)proc("glGenQueriesEXT");
    glGenVertexArraysOES = (PFNGLGENVERTEXARRAYSOESPROC)proc("glGenVertexArraysOES");
    glGetBufferPointervOES = (PFNGLGETBUFFERPOINTERVOESPROC)proc("glGetBufferPointervOES");
    glGetCoverageModulationTableNV = (PFNGLGETCOVERAGEMODULATIONTABLENVPROC)proc("glGetCoverageModulationTableNV");
    glGetDebugMessageLogKHR = (PFNGLGETDEBUGMESSAGELOGKHRPROC)proc("glGetDebugMessageLogKHR");
    glGetDriverControlStringQCOM = (PFNGLGETDRIVERCONTROLSTRINGQCOMPROC)proc("glGetDriverControlStringQCOM");
    glGetDriverControlsQCOM = (PFNGLGETDRIVERCONTROLSQCOMPROC)proc("glGetDriverControlsQCOM");
    glGetFenceivNV = (PFNGLGETFENCEIVNVPROC)proc("glGetFenceivNV");
    glGetFirstPerfQueryIdINTEL = (PFNGLGETFIRSTPERFQUERYIDINTELPROC)proc("glGetFirstPerfQueryIdINTEL");
    glGetFloati_vNV = (PFNGLGETFLOATI_VNVPROC)proc("glGetFloati_vNV");
    glGetFragDataIndexEXT = (PFNGLGETFRAGDATAINDEXEXTPROC)proc("glGetFragDataIndexEXT");
    glGetFramebufferPixelLocalStorageSizeEXT = (PFNGLGETFRAMEBUFFERPIXELLOCALSTORAGESIZEEXTPROC)proc("glGetFramebufferPixelLocalStorageSizeEXT");
    glGetGraphicsResetStatusEXT = (PFNGLGETGRAPHICSRESETSTATUSEXTPROC)proc("glGetGraphicsResetStatusEXT");
    glGetGraphicsResetStatusKHR = (PFNGLGETGRAPHICSRESETSTATUSKHRPROC)proc("glGetGraphicsResetStatusKHR");
    glGetImageHandleNV = (PFNGLGETIMAGEHANDLENVPROC)proc("glGetImageHandleNV");
    glGetInteger64vAPPLE = (PFNGLGETINTEGER64VAPPLEPROC)proc("glGetInteger64vAPPLE");
    glGetIntegeri_vEXT = (PFNGLGETINTEGERI_VEXTPROC)proc("glGetIntegeri_vEXT");
    glGetInternalformatSampleivNV = (PFNGLGETINTERNALFORMATSAMPLEIVNVPROC)proc("glGetInternalformatSampleivNV");
    glGetNextPerfQueryIdINTEL = (PFNGLGETNEXTPERFQUERYIDINTELPROC)proc("glGetNextPerfQueryIdINTEL");
    glGetObjectLabelEXT = (PFNGLGETOBJECTLABELEXTPROC)proc("glGetObjectLabelEXT");
    glGetObjectLabelKHR = (PFNGLGETOBJECTLABELKHRPROC)proc("glGetObjectLabelKHR");
    glGetObjectPtrLabelKHR = (PFNGLGETOBJECTPTRLABELKHRPROC)proc("glGetObjectPtrLabelKHR");
    glGetPathCommandsNV = (PFNGLGETPATHCOMMANDSNVPROC)proc("glGetPathCommandsNV");
    glGetPathCoordsNV = (PFNGLGETPATHCOORDSNVPROC)proc("glGetPathCoordsNV");
    glGetPathDashArrayNV = (PFNGLGETPATHDASHARRAYNVPROC)proc("glGetPathDashArrayNV");
    glGetPathLengthNV = (PFNGLGETPATHLENGTHNVPROC)proc("glGetPathLengthNV");
    glGetPathMetricRangeNV = (PFNGLGETPATHMETRICRANGENVPROC)proc("glGetPathMetricRangeNV");
    glGetPathMetricsNV = (PFNGLGETPATHMETRICSNVPROC)proc("glGetPathMetricsNV");
    glGetPathParameterfvNV = (PFNGLGETPATHPARAMETERFVNVPROC)proc("glGetPathParameterfvNV");
    glGetPathParameterivNV = (PFNGLGETPATHPARAMETERIVNVPROC)proc("glGetPathParameterivNV");
    glGetPathSpacingNV = (PFNGLGETPATHSPACINGNVPROC)proc("glGetPathSpacingNV");
    glGetPerfCounterInfoINTEL = (PFNGLGETPERFCOUNTERINFOINTELPROC)proc("glGetPerfCounterInfoINTEL");
    glGetPerfMonitorCounterDataAMD = (PFNGLGETPERFMONITORCOUNTERDATAAMDPROC)proc("glGetPerfMonitorCounterDataAMD");
    glGetPerfMonitorCounterInfoAMD = (PFNGLGETPERFMONITORCOUNTERINFOAMDPROC)proc("glGetPerfMonitorCounterInfoAMD");
    glGetPerfMonitorCounterStringAMD = (PFNGLGETPERFMONITORCOUNTERSTRINGAMDPROC)proc("glGetPerfMonitorCounterStringAMD");
    glGetPerfMonitorCountersAMD = (PFNGLGETPERFMONITORCOUNTERSAMDPROC)proc("glGetPerfMonitorCountersAMD");
    glGetPerfMonitorGroupStringAMD = (PFNGLGETPERFMONITORGROUPSTRINGAMDPROC)proc("glGetPerfMonitorGroupStringAMD");
    glGetPerfMonitorGroupsAMD = (PFNGLGETPERFMONITORGROUPSAMDPROC)proc("glGetPerfMonitorGroupsAMD");
    glGetPerfQueryDataINTEL = (PFNGLGETPERFQUERYDATAINTELPROC)proc("glGetPerfQueryDataINTEL");
    glGetPerfQueryIdByNameINTEL = (PFNGLGETPERFQUERYIDBYNAMEINTELPROC)proc("glGetPerfQueryIdByNameINTEL");
    glGetPerfQueryInfoINTEL = (PFNGLGETPERFQUERYINFOINTELPROC)proc("glGetPerfQueryInfoINTEL");
    glGetPointervKHR = (PFNGLGETPOINTERVKHRPROC)proc("glGetPointervKHR");
    glGetProgramBinaryOES = (PFNGLGETPROGRAMBINARYOESPROC)proc("glGetProgramBinaryOES");
    glGetProgramPipelineInfoLogEXT = (PFNGLGETPROGRAMPIPELINEINFOLOGEXTPROC)proc("glGetProgramPipelineInfoLogEXT");
    glGetProgramPipelineivEXT = (PFNGLGETPROGRAMPIPELINEIVEXTPROC)proc("glGetProgramPipelineivEXT");
    glGetProgramResourceLocationIndexEXT = (PFNGLGETPROGRAMRESOURCELOCATIONINDEXEXTPROC)proc("glGetProgramResourceLocationIndexEXT");
    glGetProgramResourcefvNV = (PFNGLGETPROGRAMRESOURCEFVNVPROC)proc("glGetProgramResourcefvNV");
    glGetQueryObjecti64vEXT = (PFNGLGETQUERYOBJECTI64VEXTPROC)proc("glGetQueryObjecti64vEXT");
    glGetQueryObjectivEXT = (PFNGLGETQUERYOBJECTIVEXTPROC)proc("glGetQueryObjectivEXT");
    glGetQueryObjectui64vEXT = (PFNGLGETQUERYOBJECTUI64VEXTPROC)proc("glGetQueryObjectui64vEXT");
    glGetQueryObjectuivEXT = (PFNGLGETQUERYOBJECTUIVEXTPROC)proc("glGetQueryObjectuivEXT");
    glGetQueryivEXT = (PFNGLGETQUERYIVEXTPROC)proc("glGetQueryivEXT");
    glGetSamplerParameterIivEXT = (PFNGLGETSAMPLERPARAMETERIIVEXTPROC)proc("glGetSamplerParameterIivEXT");
    glGetSamplerParameterIivOES = (PFNGLGETSAMPLERPARAMETERIIVOESPROC)proc("glGetSamplerParameterIivOES");
    glGetSamplerParameterIuivEXT = (PFNGLGETSAMPLERPARAMETERIUIVEXTPROC)proc("glGetSamplerParameterIuivEXT");
    glGetSamplerParameterIuivOES = (PFNGLGETSAMPLERPARAMETERIUIVOESPROC)proc("glGetSamplerParameterIuivOES");
    glGetSyncivAPPLE = (PFNGLGETSYNCIVAPPLEPROC)proc("glGetSyncivAPPLE");
    glGetTexParameterIivEXT = (PFNGLGETTEXPARAMETERIIVEXTPROC)proc("glGetTexParameterIivEXT");
    glGetTexParameterIivOES = (PFNGLGETTEXPARAMETERIIVOESPROC)proc("glGetTexParameterIivOES");
    glGetTexParameterIuivEXT = (PFNGLGETTEXPARAMETERIUIVEXTPROC)proc("glGetTexParameterIuivEXT");
    glGetTexParameterIuivOES = (PFNGLGETTEXPARAMETERIUIVOESPROC)proc("glGetTexParameterIuivOES");
    glGetTextureHandleNV = (PFNGLGETTEXTUREHANDLENVPROC)proc("glGetTextureHandleNV");
    glGetTextureSamplerHandleNV = (PFNGLGETTEXTURESAMPLERHANDLENVPROC)proc("glGetTextureSamplerHandleNV");
    glGetTranslatedShaderSourceANGLE = (PFNGLGETTRANSLATEDSHADERSOURCEANGLEPROC)proc("glGetTranslatedShaderSourceANGLE");
    glGetnUniformfvEXT = (PFNGLGETNUNIFORMFVEXTPROC)proc("glGetnUniformfvEXT");
    glGetnUniformfvKHR = (PFNGLGETNUNIFORMFVKHRPROC)proc("glGetnUniformfvKHR");
    glGetnUniformivEXT = (PFNGLGETNUNIFORMIVEXTPROC)proc("glGetnUniformivEXT");
    glGetnUniformivKHR = (PFNGLGETNUNIFORMIVKHRPROC)proc("glGetnUniformivKHR");
    glGetnUniformuivKHR = (PFNGLGETNUNIFORMUIVKHRPROC)proc("glGetnUniformuivKHR");
    glInsertEventMarkerEXT = (PFNGLINSERTEVENTMARKEREXTPROC)proc("glInsertEventMarkerEXT");
    glInterpolatePathsNV = (PFNGLINTERPOLATEPATHSNVPROC)proc("glInterpolatePathsNV");
    glIsEnablediEXT = (PFNGLISENABLEDIEXTPROC)proc("glIsEnablediEXT");
    glIsEnablediNV = (PFNGLISENABLEDINVPROC)proc("glIsEnablediNV");
    glIsEnablediOES = (PFNGLISENABLEDIOESPROC)proc("glIsEnablediOES");
    glIsFenceNV = (PFNGLISFENCENVPROC)proc("glIsFenceNV");
    glIsImageHandleResidentNV = (PFNGLISIMAGEHANDLERESIDENTNVPROC)proc("glIsImageHandleResidentNV");
    glIsPathNV = (PFNGLISPATHNVPROC)proc("glIsPathNV");
    glIsPointInFillPathNV = (PFNGLISPOINTINFILLPATHNVPROC)proc("glIsPointInFillPathNV");
    glIsPointInStrokePathNV = (PFNGLISPOINTINSTROKEPATHNVPROC)proc("glIsPointInStrokePathNV");
    glIsProgramPipelineEXT = (PFNGLISPROGRAMPIPELINEEXTPROC)proc("glIsProgramPipelineEXT");
    glIsQueryEXT = (PFNGLISQUERYEXTPROC)proc("glIsQueryEXT");
    glIsSyncAPPLE = (PFNGLISSYNCAPPLEPROC)proc("glIsSyncAPPLE");
    glIsTextureHandleResidentNV = (PFNGLISTEXTUREHANDLERESIDENTNVPROC)proc("glIsTextureHandleResidentNV");
    glIsVertexArrayOES = (PFNGLISVERTEXARRAYOESPROC)proc("glIsVertexArrayOES");
    glLabelObjectEXT = (PFNGLLABELOBJECTEXTPROC)proc("glLabelObjectEXT");
    glMakeImageHandleNonResidentNV = (PFNGLMAKEIMAGEHANDLENONRESIDENTNVPROC)proc("glMakeImageHandleNonResidentNV");
    glMakeImageHandleResidentNV = (PFNGLMAKEIMAGEHANDLERESIDENTNVPROC)proc("glMakeImageHandleResidentNV");
    glMakeTextureHandleNonResidentNV = (PFNGLMAKETEXTUREHANDLENONRESIDENTNVPROC)proc("glMakeTextureHandleNonResidentNV");
    glMakeTextureHandleResidentNV = (PFNGLMAKETEXTUREHANDLERESIDENTNVPROC)proc("glMakeTextureHandleResidentNV");
    glMapBufferOES = (PFNGLMAPBUFFEROESPROC)proc("glMapBufferOES");
    glMapBufferRangeEXT = (PFNGLMAPBUFFERRANGEEXTPROC)proc("glMapBufferRangeEXT");
    glMatrixLoad3x2fNV = (PFNGLMATRIXLOAD3X2FNVPROC)proc("glMatrixLoad3x2fNV");
    glMatrixLoad3x3fNV = (PFNGLMATRIXLOAD3X3FNVPROC)proc("glMatrixLoad3x3fNV");
    glMatrixLoadTranspose3x3fNV = (PFNGLMATRIXLOADTRANSPOSE3X3FNVPROC)proc("glMatrixLoadTranspose3x3fNV");
    glMatrixMult3x2fNV = (PFNGLMATRIXMULT3X2FNVPROC)proc("glMatrixMult3x2fNV");
    glMatrixMult3x3fNV = (PFNGLMATRIXMULT3X3FNVPROC)proc("glMatrixMult3x3fNV");
    glMatrixMultTranspose3x3fNV = (PFNGLMATRIXMULTTRANSPOSE3X3FNVPROC)proc("glMatrixMultTranspose3x3fNV");
    glMinSampleShadingOES = (PFNGLMINSAMPLESHADINGOESPROC)proc("glMinSampleShadingOES");
    glMultiDrawArraysEXT = (PFNGLMULTIDRAWARRAYSEXTPROC)proc("glMultiDrawArraysEXT");
    glMultiDrawArraysIndirectEXT = (PFNGLMULTIDRAWARRAYSINDIRECTEXTPROC)proc("glMultiDrawArraysIndirectEXT");
    glMultiDrawElementsBaseVertexEXT = (PFNGLMULTIDRAWELEMENTSBASEVERTEXEXTPROC)proc("glMultiDrawElementsBaseVertexEXT");
    glMultiDrawElementsBaseVertexOES = (PFNGLMULTIDRAWELEMENTSBASEVERTEXOESPROC)proc("glMultiDrawElementsBaseVertexOES");
    glMultiDrawElementsEXT = (PFNGLMULTIDRAWELEMENTSEXTPROC)proc("glMultiDrawElementsEXT");
    glMultiDrawElementsIndirectEXT = (PFNGLMULTIDRAWELEMENTSINDIRECTEXTPROC)proc("glMultiDrawElementsIndirectEXT");
    glNamedFramebufferSampleLocationsfvNV = (PFNGLNAMEDFRAMEBUFFERSAMPLELOCATIONSFVNVPROC)proc("glNamedFramebufferSampleLocationsfvNV");
    glObjectLabelKHR = (PFNGLOBJECTLABELKHRPROC)proc("glObjectLabelKHR");
    glObjectPtrLabelKHR = (PFNGLOBJECTPTRLABELKHRPROC)proc("glObjectPtrLabelKHR");
    glPatchParameteriEXT = (PFNGLPATCHPARAMETERIEXTPROC)proc("glPatchParameteriEXT");
    glPatchParameteriOES = (PFNGLPATCHPARAMETERIOESPROC)proc("glPatchParameteriOES");
    glPathCommandsNV = (PFNGLPATHCOMMANDSNVPROC)proc("glPathCommandsNV");
    glPathCoordsNV = (PFNGLPATHCOORDSNVPROC)proc("glPathCoordsNV");
    glPathCoverDepthFuncNV = (PFNGLPATHCOVERDEPTHFUNCNVPROC)proc("glPathCoverDepthFuncNV");
    glPathDashArrayNV = (PFNGLPATHDASHARRAYNVPROC)proc("glPathDashArrayNV");
    glPathGlyphIndexArrayNV = (PFNGLPATHGLYPHINDEXARRAYNVPROC)proc("glPathGlyphIndexArrayNV");
    glPathGlyphIndexRangeNV = (PFNGLPATHGLYPHINDEXRANGENVPROC)proc("glPathGlyphIndexRangeNV");
    glPathGlyphRangeNV = (PFNGLPATHGLYPHRANGENVPROC)proc("glPathGlyphRangeNV");
    glPathGlyphsNV = (PFNGLPATHGLYPHSNVPROC)proc("glPathGlyphsNV");
    glPathMemoryGlyphIndexArrayNV = (PFNGLPATHMEMORYGLYPHINDEXARRAYNVPROC)proc("glPathMemoryGlyphIndexArrayNV");
    glPathParameterfNV = (PFNGLPATHPARAMETERFNVPROC)proc("glPathParameterfNV");
    glPathParameterfvNV = (PFNGLPATHPARAMETERFVNVPROC)proc("glPathParameterfvNV");
    glPathParameteriNV = (PFNGLPATHPARAMETERINVPROC)proc("glPathParameteriNV");
    glPathParameterivNV = (PFNGLPATHPARAMETERIVNVPROC)proc("glPathParameterivNV");
    glPathStencilDepthOffsetNV = (PFNGLPATHSTENCILDEPTHOFFSETNVPROC)proc("glPathStencilDepthOffsetNV");
    glPathStencilFuncNV = (PFNGLPATHSTENCILFUNCNVPROC)proc("glPathStencilFuncNV");
    glPathStringNV = (PFNGLPATHSTRINGNVPROC)proc("glPathStringNV");
    glPathSubCommandsNV = (PFNGLPATHSUBCOMMANDSNVPROC)proc("glPathSubCommandsNV");
    glPathSubCoordsNV = (PFNGLPATHSUBCOORDSNVPROC)proc("glPathSubCoordsNV");
    glPointAlongPathNV = (PFNGLPOINTALONGPATHNVPROC)proc("glPointAlongPathNV");
    glPolygonModeNV = (PFNGLPOLYGONMODENVPROC)proc("glPolygonModeNV");
    glPolygonOffsetClampEXT = (PFNGLPOLYGONOFFSETCLAMPEXTPROC)proc("glPolygonOffsetClampEXT");
    glPopDebugGroupKHR = (PFNGLPOPDEBUGGROUPKHRPROC)proc("glPopDebugGroupKHR");
    glPopGroupMarkerEXT = (PFNGLPOPGROUPMARKEREXTPROC)proc("glPopGroupMarkerEXT");
    glPrimitiveBoundingBoxEXT = (PFNGLPRIMITIVEBOUNDINGBOXEXTPROC)proc("glPrimitiveBoundingBoxEXT");
    glPrimitiveBoundingBoxOES = (PFNGLPRIMITIVEBOUNDINGBOXOESPROC)proc("glPrimitiveBoundingBoxOES");
    glProgramBinaryOES = (PFNGLPROGRAMBINARYOESPROC)proc("glProgramBinaryOES");
    glProgramParameteriEXT = (PFNGLPROGRAMPARAMETERIEXTPROC)proc("glProgramParameteriEXT");
    glProgramPathFragmentInputGenNV = (PFNGLPROGRAMPATHFRAGMENTINPUTGENNVPROC)proc("glProgramPathFragmentInputGenNV");
    glProgramUniform1fEXT = (PFNGLPROGRAMUNIFORM1FEXTPROC)proc("glProgramUniform1fEXT");
    glProgramUniform1fvEXT = (PFNGLPROGRAMUNIFORM1FVEXTPROC)proc("glProgramUniform1fvEXT");
    glProgramUniform1iEXT = (PFNGLPROGRAMUNIFORM1IEXTPROC)proc("glProgramUniform1iEXT");
    glProgramUniform1ivEXT = (PFNGLPROGRAMUNIFORM1IVEXTPROC)proc("glProgramUniform1ivEXT");
    glProgramUniform1uiEXT = (PFNGLPROGRAMUNIFORM1UIEXTPROC)proc("glProgramUniform1uiEXT");
    glProgramUniform1uivEXT = (PFNGLPROGRAMUNIFORM1UIVEXTPROC)proc("glProgramUniform1uivEXT");
    glProgramUniform2fEXT = (PFNGLPROGRAMUNIFORM2FEXTPROC)proc("glProgramUniform2fEXT");
    glProgramUniform2fvEXT = (PFNGLPROGRAMUNIFORM2FVEXTPROC)proc("glProgramUniform2fvEXT");
    glProgramUniform2iEXT = (PFNGLPROGRAMUNIFORM2IEXTPROC)proc("glProgramUniform2iEXT");
    glProgramUniform2ivEXT = (PFNGLPROGRAMUNIFORM2IVEXTPROC)proc("glProgramUniform2ivEXT");
    glProgramUniform2uiEXT = (PFNGLPROGRAMUNIFORM2UIEXTPROC)proc("glProgramUniform2uiEXT");
    glProgramUniform2uivEXT = (PFNGLPROGRAMUNIFORM2UIVEXTPROC)proc("glProgramUniform2uivEXT");
    glProgramUniform3fEXT = (PFNGLPROGRAMUNIFORM3FEXTPROC)proc("glProgramUniform3fEXT");
    glProgramUniform3fvEXT = (PFNGLPROGRAMUNIFORM3FVEXTPROC)proc("glProgramUniform3fvEXT");
    glProgramUniform3iEXT = (PFNGLPROGRAMUNIFORM3IEXTPROC)proc("glProgramUniform3iEXT");
    glProgramUniform3ivEXT = (PFNGLPROGRAMUNIFORM3IVEXTPROC)proc("glProgramUniform3ivEXT");
    glProgramUniform3uiEXT = (PFNGLPROGRAMUNIFORM3UIEXTPROC)proc("glProgramUniform3uiEXT");
    glProgramUniform3uivEXT = (PFNGLPROGRAMUNIFORM3UIVEXTPROC)proc("glProgramUniform3uivEXT");
    glProgramUniform4fEXT = (PFNGLPROGRAMUNIFORM4FEXTPROC)proc("glProgramUniform4fEXT");
    glProgramUniform4fvEXT = (PFNGLPROGRAMUNIFORM4FVEXTPROC)proc("glProgramUniform4fvEXT");
    glProgramUniform4iEXT = (PFNGLPROGRAMUNIFORM4IEXTPROC)proc("glProgramUniform4iEXT");
    glProgramUniform4ivEXT = (PFNGLPROGRAMUNIFORM4IVEXTPROC)proc("glProgramUniform4ivEXT");
    glProgramUniform4uiEXT = (PFNGLPROGRAMUNIFORM4UIEXTPROC)proc("glProgramUniform4uiEXT");
    glProgramUniform4uivEXT = (PFNGLPROGRAMUNIFORM4UIVEXTPROC)proc("glProgramUniform4uivEXT");
    glProgramUniformHandleui64NV = (PFNGLPROGRAMUNIFORMHANDLEUI64NVPROC)proc("glProgramUniformHandleui64NV");
    glProgramUniformHandleui64vNV = (PFNGLPROGRAMUNIFORMHANDLEUI64VNVPROC)proc("glProgramUniformHandleui64vNV");
    glProgramUniformMatrix2fvEXT = (PFNGLPROGRAMUNIFORMMATRIX2FVEXTPROC)proc("glProgramUniformMatrix2fvEXT");
    glProgramUniformMatrix2x3fvEXT = (PFNGLPROGRAMUNIFORMMATRIX2X3FVEXTPROC)proc("glProgramUniformMatrix2x3fvEXT");
    glProgramUniformMatrix2x4fvEXT = (PFNGLPROGRAMUNIFORMMATRIX2X4FVEXTPROC)proc("glProgramUniformMatrix2x4fvEXT");
    glProgramUniformMatrix3fvEXT = (PFNGLPROGRAMUNIFORMMATRIX3FVEXTPROC)proc("glProgramUniformMatrix3fvEXT");
    glProgramUniformMatrix3x2fvEXT = (PFNGLPROGRAMUNIFORMMATRIX3X2FVEXTPROC)proc("glProgramUniformMatrix3x2fvEXT");
    glProgramUniformMatrix3x4fvEXT = (PFNGLPROGRAMUNIFORMMATRIX3X4FVEXTPROC)proc("glProgramUniformMatrix3x4fvEXT");
    glProgramUniformMatrix4fvEXT = (PFNGLPROGRAMUNIFORMMATRIX4FVEXTPROC)proc("glProgramUniformMatrix4fvEXT");
    glProgramUniformMatrix4x2fvEXT = (PFNGLPROGRAMUNIFORMMATRIX4X2FVEXTPROC)proc("glProgramUniformMatrix4x2fvEXT");
    glProgramUniformMatrix4x3fvEXT = (PFNGLPROGRAMUNIFORMMATRIX4X3FVEXTPROC)proc("glProgramUniformMatrix4x3fvEXT");
    glPushDebugGroupKHR = (PFNGLPUSHDEBUGGROUPKHRPROC)proc("glPushDebugGroupKHR");
    glPushGroupMarkerEXT = (PFNGLPUSHGROUPMARKEREXTPROC)proc("glPushGroupMarkerEXT");
    glQueryCounterEXT = (PFNGLQUERYCOUNTEREXTPROC)proc("glQueryCounterEXT");
    glRasterSamplesEXT = (PFNGLRASTERSAMPLESEXTPROC)proc("glRasterSamplesEXT");
    glReadBufferIndexedEXT = (PFNGLREADBUFFERINDEXEDEXTPROC)proc("glReadBufferIndexedEXT");
    glReadBufferNV = (PFNGLREADBUFFERNVPROC)proc("glReadBufferNV");
    glReadnPixelsEXT = (PFNGLREADNPIXELSEXTPROC)proc("glReadnPixelsEXT");
    glReadnPixelsKHR = (PFNGLREADNPIXELSKHRPROC)proc("glReadnPixelsKHR");
    glRenderbufferStorageMultisampleANGLE = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEANGLEPROC)proc("glRenderbufferStorageMultisampleANGLE");
    glRenderbufferStorageMultisampleAPPLE = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEAPPLEPROC)proc("glRenderbufferStorageMultisampleAPPLE");
    glRenderbufferStorageMultisampleEXT = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)proc("glRenderbufferStorageMultisampleEXT");
    glRenderbufferStorageMultisampleIMG = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMGPROC)proc("glRenderbufferStorageMultisampleIMG");
    glRenderbufferStorageMultisampleNV = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLENVPROC)proc("glRenderbufferStorageMultisampleNV");
    glResolveDepthValuesNV = (PFNGLRESOLVEDEPTHVALUESNVPROC)proc("glResolveDepthValuesNV");
    glResolveMultisampleFramebufferAPPLE = (PFNGLRESOLVEMULTISAMPLEFRAMEBUFFERAPPLEPROC)proc("glResolveMultisampleFramebufferAPPLE");
    glSamplerParameterIivEXT = (PFNGLSAMPLERPARAMETERIIVEXTPROC)proc("glSamplerParameterIivEXT");
    glSamplerParameterIivOES = (PFNGLSAMPLERPARAMETERIIVOESPROC)proc("glSamplerParameterIivOES");
    glSamplerParameterIuivEXT = (PFNGLSAMPLERPARAMETERIUIVEXTPROC)proc("glSamplerParameterIuivEXT");
    glSamplerParameterIuivOES = (PFNGLSAMPLERPARAMETERIUIVOESPROC)proc("glSamplerParameterIuivOES");
    glScissorArrayvNV = (PFNGLSCISSORARRAYVNVPROC)proc("glScissorArrayvNV");
    glScissorIndexedNV = (PFNGLSCISSORINDEXEDNVPROC)proc("glScissorIndexedNV");
    glScissorIndexedvNV = (PFNGLSCISSORINDEXEDVNVPROC)proc("glScissorIndexedvNV");
    glSelectPerfMonitorCountersAMD = (PFNGLSELECTPERFMONITORCOUNTERSAMDPROC)proc("glSelectPerfMonitorCountersAMD");
    glSetFenceNV = (PFNGLSETFENCENVPROC)proc("glSetFenceNV");
    glStartTilingQCOM = (PFNGLSTARTTILINGQCOMPROC)proc("glStartTilingQCOM");
    glStencilFillPathInstancedNV = (PFNGLSTENCILFILLPATHINSTANCEDNVPROC)proc("glStencilFillPathInstancedNV");
    glStencilFillPathNV = (PFNGLSTENCILFILLPATHNVPROC)proc("glStencilFillPathNV");
    glStencilStrokePathInstancedNV = (PFNGLSTENCILSTROKEPATHINSTANCEDNVPROC)proc("glStencilStrokePathInstancedNV");
    glStencilStrokePathNV = (PFNGLSTENCILSTROKEPATHNVPROC)proc("glStencilStrokePathNV");
    glStencilThenCoverFillPathInstancedNV = (PFNGLSTENCILTHENCOVERFILLPATHINSTANCEDNVPROC)proc("glStencilThenCoverFillPathInstancedNV");
    glStencilThenCoverFillPathNV = (PFNGLSTENCILTHENCOVERFILLPATHNVPROC)proc("glStencilThenCoverFillPathNV");
    glStencilThenCoverStrokePathInstancedNV = (PFNGLSTENCILTHENCOVERSTROKEPATHINSTANCEDNVPROC)proc("glStencilThenCoverStrokePathInstancedNV");
    glStencilThenCoverStrokePathNV = (PFNGLSTENCILTHENCOVERSTROKEPATHNVPROC)proc("glStencilThenCoverStrokePathNV");
    glSubpixelPrecisionBiasNV = (PFNGLSUBPIXELPRECISIONBIASNVPROC)proc("glSubpixelPrecisionBiasNV");
    glTestFenceNV = (PFNGLTESTFENCENVPROC)proc("glTestFenceNV");
    glTexBufferEXT = (PFNGLTEXBUFFEREXTPROC)proc("glTexBufferEXT");
    glTexBufferOES = (PFNGLTEXBUFFEROESPROC)proc("glTexBufferOES");
    glTexBufferRangeEXT = (PFNGLTEXBUFFERRANGEEXTPROC)proc("glTexBufferRangeEXT");
    glTexBufferRangeOES = (PFNGLTEXBUFFERRANGEOESPROC)proc("glTexBufferRangeOES");
    glTexImage3DOES = (PFNGLTEXIMAGE3DOESPROC)proc("glTexImage3DOES");
    glTexPageCommitmentEXT = (PFNGLTEXPAGECOMMITMENTEXTPROC)proc("glTexPageCommitmentEXT");
    glTexParameterIivEXT = (PFNGLTEXPARAMETERIIVEXTPROC)proc("glTexParameterIivEXT");
    glTexParameterIivOES = (PFNGLTEXPARAMETERIIVOESPROC)proc("glTexParameterIivOES");
    glTexParameterIuivEXT = (PFNGLTEXPARAMETERIUIVEXTPROC)proc("glTexParameterIuivEXT");
    glTexParameterIuivOES = (PFNGLTEXPARAMETERIUIVOESPROC)proc("glTexParameterIuivOES");
    glTexStorage1DEXT = (PFNGLTEXSTORAGE1DEXTPROC)proc("glTexStorage1DEXT");
    glTexStorage2DEXT = (PFNGLTEXSTORAGE2DEXTPROC)proc("glTexStorage2DEXT");
    glTexStorage3DEXT = (PFNGLTEXSTORAGE3DEXTPROC)proc("glTexStorage3DEXT");
    glTexStorage3DMultisampleOES = (PFNGLTEXSTORAGE3DMULTISAMPLEOESPROC)proc("glTexStorage3DMultisampleOES");
    glTexSubImage3DOES = (PFNGLTEXSUBIMAGE3DOESPROC)proc("glTexSubImage3DOES");
    glTextureStorage1DEXT = (PFNGLTEXTURESTORAGE1DEXTPROC)proc("glTextureStorage1DEXT");
    glTextureStorage2DEXT = (PFNGLTEXTURESTORAGE2DEXTPROC)proc("glTextureStorage2DEXT");
    glTextureStorage3DEXT = (PFNGLTEXTURESTORAGE3DEXTPROC)proc("glTextureStorage3DEXT");
    glTextureViewEXT = (PFNGLTEXTUREVIEWEXTPROC)proc("glTextureViewEXT");
    glTextureViewOES = (PFNGLTEXTUREVIEWOESPROC)proc("glTextureViewOES");
    glTransformPathNV = (PFNGLTRANSFORMPATHNVPROC)proc("glTransformPathNV");
    glUniformHandleui64NV = (PFNGLUNIFORMHANDLEUI64NVPROC)proc("glUniformHandleui64NV");
    glUniformHandleui64vNV = (PFNGLUNIFORMHANDLEUI64VNVPROC)proc("glUniformHandleui64vNV");
    glUniformMatrix2x3fvNV = (PFNGLUNIFORMMATRIX2X3FVNVPROC)proc("glUniformMatrix2x3fvNV");
    glUniformMatrix2x4fvNV = (PFNGLUNIFORMMATRIX2X4FVNVPROC)proc("glUniformMatrix2x4fvNV");
    glUniformMatrix3x2fvNV = (PFNGLUNIFORMMATRIX3X2FVNVPROC)proc("glUniformMatrix3x2fvNV");
    glUniformMatrix3x4fvNV = (PFNGLUNIFORMMATRIX3X4FVNVPROC)proc("glUniformMatrix3x4fvNV");
    glUniformMatrix4x2fvNV = (PFNGLUNIFORMMATRIX4X2FVNVPROC)proc("glUniformMatrix4x2fvNV");
    glUniformMatrix4x3fvNV = (PFNGLUNIFORMMATRIX4X3FVNVPROC)proc("glUniformMatrix4x3fvNV");
    glUnmapBufferOES = (PFNGLUNMAPBUFFEROESPROC)proc("glUnmapBufferOES");
    glUseProgramStagesEXT = (PFNGLUSEPROGRAMSTAGESEXTPROC)proc("glUseProgramStagesEXT");
    glValidateProgramPipelineEXT = (PFNGLVALIDATEPROGRAMPIPELINEEXTPROC)proc("glValidateProgramPipelineEXT");
    glVertexAttribDivisorANGLE = (PFNGLVERTEXATTRIBDIVISORANGLEPROC)proc("glVertexAttribDivisorANGLE");
    glVertexAttribDivisorEXT = (PFNGLVERTEXATTRIBDIVISOREXTPROC)proc("glVertexAttribDivisorEXT");
    glVertexAttribDivisorNV = (PFNGLVERTEXATTRIBDIVISORNVPROC)proc("glVertexAttribDivisorNV");
    glViewportArrayvNV = (PFNGLVIEWPORTARRAYVNVPROC)proc("glViewportArrayvNV");
    glViewportIndexedfNV = (PFNGLVIEWPORTINDEXEDFNVPROC)proc("glViewportIndexedfNV");
    glViewportIndexedfvNV = (PFNGLVIEWPORTINDEXEDFVNVPROC)proc("glViewportIndexedfvNV");
    glWaitSyncAPPLE = (PFNGLWAITSYNCAPPLEPROC)proc("glWaitSyncAPPLE");
    glWeightPathsNV = (PFNGLWEIGHTPATHSNVPROC)proc("glWeightPathsNV");
}

//#endif

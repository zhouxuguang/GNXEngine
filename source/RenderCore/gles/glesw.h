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

//启用opengl扩展函数，这里需要注意的是避免和系统头文件冲突
#define ENABLE_GLEXT_FUNC

#ifdef ENABLE_GLEXT_FUNC

#ifndef __glesw_h_
#define __glesw_h_

#include "khrplatform.h"
#include "gl3platform.h"
#include "gl2ext.h"

#ifdef __cplusplus
extern "C" {
#endif
    
    typedef void (*GLESWglProc)(void);
    typedef GLESWglProc (*glGetProcAddressProc)(const char *proc);
    
    /* gl api */
    int gleswInit(void);
    int gleswInit2(glGetProcAddressProc proc);
    int gleswIsSupported(int major, int minor);
    GLESWglProc glGetProcAddress(const char *proc);
    
#if 1
    /* OpenGL functions */
    extern PFNGLACTIVESHADERPROGRAMEXTPROC                         glActiveShaderProgramEXT;
    extern PFNGLALPHAFUNCQCOMPROC                                  glAlphaFuncQCOM;
    extern PFNGLAPPLYFRAMEBUFFERATTACHMENTCMAAINTELPROC            glApplyFramebufferAttachmentCMAAINTEL;
    extern PFNGLBEGINCONDITIONALRENDERNVPROC                       glBeginConditionalRenderNV;
    extern PFNGLBEGINPERFMONITORAMDPROC                            glBeginPerfMonitorAMD;
    extern PFNGLBEGINPERFQUERYINTELPROC                            glBeginPerfQueryINTEL;
    extern PFNGLBEGINQUERYEXTPROC                                  glBeginQueryEXT;
    extern PFNGLBINDFRAGDATALOCATIONEXTPROC                        glBindFragDataLocationEXT;
    extern PFNGLBINDFRAGDATALOCATIONINDEXEDEXTPROC                 glBindFragDataLocationIndexedEXT;
    extern PFNGLBINDPROGRAMPIPELINEEXTPROC                         glBindProgramPipelineEXT;
    extern PFNGLBINDVERTEXARRAYOESPROC                             glBindVertexArrayOES;
    extern PFNGLBLENDBARRIERKHRPROC                                glBlendBarrierKHR;
    extern PFNGLBLENDBARRIERNVPROC                                 glBlendBarrierNV;
    extern PFNGLBLENDEQUATIONSEPARATEIEXTPROC                      glBlendEquationSeparateiEXT;
    extern PFNGLBLENDEQUATIONSEPARATEIOESPROC                      glBlendEquationSeparateiOES;
    extern PFNGLBLENDEQUATIONIEXTPROC                              glBlendEquationiEXT;
    extern PFNGLBLENDEQUATIONIOESPROC                              glBlendEquationiOES;
    extern PFNGLBLENDFUNCSEPARATEIEXTPROC                          glBlendFuncSeparateiEXT;
    extern PFNGLBLENDFUNCSEPARATEIOESPROC                          glBlendFuncSeparateiOES;
    extern PFNGLBLENDFUNCIEXTPROC                                  glBlendFunciEXT;
    extern PFNGLBLENDFUNCIOESPROC                                  glBlendFunciOES;
    extern PFNGLBLENDPARAMETERINVPROC                              glBlendParameteriNV;
    extern PFNGLBLITFRAMEBUFFERANGLEPROC                           glBlitFramebufferANGLE;
    extern PFNGLBLITFRAMEBUFFERNVPROC                              glBlitFramebufferNV;
    extern PFNGLBUFFERSTORAGEEXTPROC                               glBufferStorageEXT;
    extern PFNGLCLEARPIXELLOCALSTORAGEUIEXTPROC                    glClearPixelLocalStorageuiEXT;
    extern PFNGLCLIENTWAITSYNCAPPLEPROC                            glClientWaitSyncAPPLE;
    extern PFNGLCOLORMASKIEXTPROC                                  glColorMaskiEXT;
    extern PFNGLCOLORMASKIOESPROC                                  glColorMaskiOES;
    extern PFNGLCOMPRESSEDTEXIMAGE3DOESPROC                        glCompressedTexImage3DOES;
    extern PFNGLCOMPRESSEDTEXSUBIMAGE3DOESPROC                     glCompressedTexSubImage3DOES;
    extern PFNGLCOPYBUFFERSUBDATANVPROC                            glCopyBufferSubDataNV;
    extern PFNGLCOPYIMAGESUBDATAEXTPROC                            glCopyImageSubDataEXT;
    extern PFNGLCOPYIMAGESUBDATAOESPROC                            glCopyImageSubDataOES;
    extern PFNGLCOPYPATHNVPROC                                     glCopyPathNV;
    extern PFNGLCOPYTEXSUBIMAGE3DOESPROC                           glCopyTexSubImage3DOES;
    extern PFNGLCOPYTEXTURELEVELSAPPLEPROC                         glCopyTextureLevelsAPPLE;
    extern PFNGLCOVERFILLPATHINSTANCEDNVPROC                       glCoverFillPathInstancedNV;
    extern PFNGLCOVERFILLPATHNVPROC                                glCoverFillPathNV;
    extern PFNGLCOVERSTROKEPATHINSTANCEDNVPROC                     glCoverStrokePathInstancedNV;
    extern PFNGLCOVERSTROKEPATHNVPROC                              glCoverStrokePathNV;
    extern PFNGLCOVERAGEMASKNVPROC                                 glCoverageMaskNV;
    extern PFNGLCOVERAGEMODULATIONNVPROC                           glCoverageModulationNV;
    extern PFNGLCOVERAGEMODULATIONTABLENVPROC                      glCoverageModulationTableNV;
    extern PFNGLCOVERAGEOPERATIONNVPROC                            glCoverageOperationNV;
    extern PFNGLCREATEPERFQUERYINTELPROC                           glCreatePerfQueryINTEL;
    extern PFNGLCREATESHADERPROGRAMVEXTPROC                        glCreateShaderProgramvEXT;
    extern PFNGLDEBUGMESSAGECALLBACKKHRPROC                        glDebugMessageCallbackKHR;
    extern PFNGLDEBUGMESSAGECONTROLKHRPROC                         glDebugMessageControlKHR;
    extern PFNGLDEBUGMESSAGEINSERTKHRPROC                          glDebugMessageInsertKHR;
    extern PFNGLDELETEFENCESNVPROC                                 glDeleteFencesNV;
    extern PFNGLDELETEPATHSNVPROC                                  glDeletePathsNV;
    extern PFNGLDELETEPERFMONITORSAMDPROC                          glDeletePerfMonitorsAMD;
    extern PFNGLDELETEPERFQUERYINTELPROC                           glDeletePerfQueryINTEL;
    extern PFNGLDELETEPROGRAMPIPELINESEXTPROC                      glDeleteProgramPipelinesEXT;
    extern PFNGLDELETEQUERIESEXTPROC                               glDeleteQueriesEXT;
    extern PFNGLDELETESYNCAPPLEPROC                                glDeleteSyncAPPLE;
    extern PFNGLDELETEVERTEXARRAYSOESPROC                          glDeleteVertexArraysOES;
    extern PFNGLDEPTHRANGEARRAYFVNVPROC                            glDepthRangeArrayfvNV;
    extern PFNGLDEPTHRANGEINDEXEDFNVPROC                           glDepthRangeIndexedfNV;
    extern PFNGLDISABLEDRIVERCONTROLQCOMPROC                       glDisableDriverControlQCOM;
    extern PFNGLDISABLEIEXTPROC                                    glDisableiEXT;
    extern PFNGLDISABLEINVPROC                                     glDisableiNV;
    extern PFNGLDISABLEIOESPROC                                    glDisableiOES;
    extern PFNGLDISCARDFRAMEBUFFEREXTPROC                          glDiscardFramebufferEXT;
    extern PFNGLDRAWARRAYSINSTANCEDANGLEPROC                       glDrawArraysInstancedANGLE;
    extern PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEEXTPROC             glDrawArraysInstancedBaseInstanceEXT;
    extern PFNGLDRAWARRAYSINSTANCEDEXTPROC                         glDrawArraysInstancedEXT;
    extern PFNGLDRAWARRAYSINSTANCEDNVPROC                          glDrawArraysInstancedNV;
    extern PFNGLDRAWBUFFERSEXTPROC                                 glDrawBuffersEXT;
    extern PFNGLDRAWBUFFERSINDEXEDEXTPROC                          glDrawBuffersIndexedEXT;
    extern PFNGLDRAWBUFFERSNVPROC                                  glDrawBuffersNV;
    extern PFNGLDRAWELEMENTSBASEVERTEXEXTPROC                      glDrawElementsBaseVertexEXT;
    extern PFNGLDRAWELEMENTSBASEVERTEXOESPROC                      glDrawElementsBaseVertexOES;
    extern PFNGLDRAWELEMENTSINSTANCEDANGLEPROC                     glDrawElementsInstancedANGLE;
    extern PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEEXTPROC           glDrawElementsInstancedBaseInstanceEXT;
    extern PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEEXTPROC glDrawElementsInstancedBaseVertexBaseInstanceEXT;
    extern PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXEXTPROC             glDrawElementsInstancedBaseVertexEXT;
    extern PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXOESPROC             glDrawElementsInstancedBaseVertexOES;
    extern PFNGLDRAWELEMENTSINSTANCEDEXTPROC                       glDrawElementsInstancedEXT;
    extern PFNGLDRAWELEMENTSINSTANCEDNVPROC                        glDrawElementsInstancedNV;
    extern PFNGLDRAWRANGEELEMENTSBASEVERTEXEXTPROC                 glDrawRangeElementsBaseVertexEXT;
    extern PFNGLDRAWRANGEELEMENTSBASEVERTEXOESPROC                 glDrawRangeElementsBaseVertexOES;
    extern PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC           glEGLImageTargetRenderbufferStorageOES;
    extern PFNGLEGLIMAGETARGETTEXTURE2DOESPROC                     glEGLImageTargetTexture2DOES;
    extern PFNGLENABLEDRIVERCONTROLQCOMPROC                        glEnableDriverControlQCOM;
    extern PFNGLENABLEIEXTPROC                                     glEnableiEXT;
    extern PFNGLENABLEINVPROC                                      glEnableiNV;
    extern PFNGLENABLEIOESPROC                                     glEnableiOES;
    extern PFNGLENDCONDITIONALRENDERNVPROC                         glEndConditionalRenderNV;
    extern PFNGLENDPERFMONITORAMDPROC                              glEndPerfMonitorAMD;
    extern PFNGLENDPERFQUERYINTELPROC                              glEndPerfQueryINTEL;
    extern PFNGLENDQUERYEXTPROC                                    glEndQueryEXT;
    extern PFNGLENDTILINGQCOMPROC                                  glEndTilingQCOM;
    extern PFNGLEXTGETBUFFERPOINTERVQCOMPROC                       glExtGetBufferPointervQCOM;
    extern PFNGLEXTGETBUFFERSQCOMPROC                              glExtGetBuffersQCOM;
    extern PFNGLEXTGETFRAMEBUFFERSQCOMPROC                         glExtGetFramebuffersQCOM;
    extern PFNGLEXTGETPROGRAMBINARYSOURCEQCOMPROC                  glExtGetProgramBinarySourceQCOM;
    extern PFNGLEXTGETPROGRAMSQCOMPROC                             glExtGetProgramsQCOM;
    extern PFNGLEXTGETRENDERBUFFERSQCOMPROC                        glExtGetRenderbuffersQCOM;
    extern PFNGLEXTGETSHADERSQCOMPROC                              glExtGetShadersQCOM;
    extern PFNGLEXTGETTEXLEVELPARAMETERIVQCOMPROC                  glExtGetTexLevelParameterivQCOM;
    extern PFNGLEXTGETTEXSUBIMAGEQCOMPROC                          glExtGetTexSubImageQCOM;
    extern PFNGLEXTGETTEXTURESQCOMPROC                             glExtGetTexturesQCOM;
    extern PFNGLEXTISPROGRAMBINARYQCOMPROC                         glExtIsProgramBinaryQCOM;
    extern PFNGLEXTTEXOBJECTSTATEOVERRIDEIQCOMPROC                 glExtTexObjectStateOverrideiQCOM;
    extern PFNGLFENCESYNCAPPLEPROC                                 glFenceSyncAPPLE;
    extern PFNGLFINISHFENCENVPROC                                  glFinishFenceNV;
    extern PFNGLFLUSHMAPPEDBUFFERRANGEEXTPROC                      glFlushMappedBufferRangeEXT;
    extern PFNGLFRAGMENTCOVERAGECOLORNVPROC                        glFragmentCoverageColorNV;
    extern PFNGLFRAMEBUFFERPIXELLOCALSTORAGESIZEEXTPROC            glFramebufferPixelLocalStorageSizeEXT;
    extern PFNGLFRAMEBUFFERSAMPLELOCATIONSFVNVPROC                 glFramebufferSampleLocationsfvNV;
    extern PFNGLFRAMEBUFFERTEXTURE2DDOWNSAMPLEIMGPROC              glFramebufferTexture2DDownsampleIMG;
    extern PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC             glFramebufferTexture2DMultisampleEXT;
    extern PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMGPROC             glFramebufferTexture2DMultisampleIMG;
    extern PFNGLFRAMEBUFFERTEXTURE3DOESPROC                        glFramebufferTexture3DOES;
    extern PFNGLFRAMEBUFFERTEXTUREEXTPROC                          glFramebufferTextureEXT;
    extern PFNGLFRAMEBUFFERTEXTURELAYERDOWNSAMPLEIMGPROC           glFramebufferTextureLayerDownsampleIMG;
    extern PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC      glFramebufferTextureMultisampleMultiviewOVR;
    extern PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC                 glFramebufferTextureMultiviewOVR;
    extern PFNGLFRAMEBUFFERTEXTUREOESPROC                          glFramebufferTextureOES;
    extern PFNGLGENFENCESNVPROC                                    glGenFencesNV;
    extern PFNGLGENPATHSNVPROC                                     glGenPathsNV;
    extern PFNGLGENPERFMONITORSAMDPROC                             glGenPerfMonitorsAMD;
    extern PFNGLGENPROGRAMPIPELINESEXTPROC                         glGenProgramPipelinesEXT;
    extern PFNGLGENQUERIESEXTPROC                                  glGenQueriesEXT;
    extern PFNGLGENVERTEXARRAYSOESPROC                             glGenVertexArraysOES;
    extern PFNGLGETBUFFERPOINTERVOESPROC                           glGetBufferPointervOES;
    extern PFNGLGETCOVERAGEMODULATIONTABLENVPROC                   glGetCoverageModulationTableNV;
    extern PFNGLGETDEBUGMESSAGELOGKHRPROC                          glGetDebugMessageLogKHR;
    extern PFNGLGETDRIVERCONTROLSTRINGQCOMPROC                     glGetDriverControlStringQCOM;
    extern PFNGLGETDRIVERCONTROLSQCOMPROC                          glGetDriverControlsQCOM;
    extern PFNGLGETFENCEIVNVPROC                                   glGetFenceivNV;
    extern PFNGLGETFIRSTPERFQUERYIDINTELPROC                       glGetFirstPerfQueryIdINTEL;
    extern PFNGLGETFLOATI_VNVPROC                                  glGetFloati_vNV;
    extern PFNGLGETFRAGDATAINDEXEXTPROC                            glGetFragDataIndexEXT;
    extern PFNGLGETFRAMEBUFFERPIXELLOCALSTORAGESIZEEXTPROC         glGetFramebufferPixelLocalStorageSizeEXT;
    extern PFNGLGETGRAPHICSRESETSTATUSEXTPROC                      glGetGraphicsResetStatusEXT;
    extern PFNGLGETGRAPHICSRESETSTATUSKHRPROC                      glGetGraphicsResetStatusKHR;
    extern PFNGLGETIMAGEHANDLENVPROC                               glGetImageHandleNV;
    extern PFNGLGETINTEGER64VAPPLEPROC                             glGetInteger64vAPPLE;
    extern PFNGLGETINTEGERI_VEXTPROC                               glGetIntegeri_vEXT;
    extern PFNGLGETINTERNALFORMATSAMPLEIVNVPROC                    glGetInternalformatSampleivNV;
    extern PFNGLGETNEXTPERFQUERYIDINTELPROC                        glGetNextPerfQueryIdINTEL;
    extern PFNGLGETOBJECTLABELEXTPROC                              glGetObjectLabelEXT;
    extern PFNGLGETOBJECTLABELKHRPROC                              glGetObjectLabelKHR;
    extern PFNGLGETOBJECTPTRLABELKHRPROC                           glGetObjectPtrLabelKHR;
    extern PFNGLGETPATHCOMMANDSNVPROC                              glGetPathCommandsNV;
    extern PFNGLGETPATHCOORDSNVPROC                                glGetPathCoordsNV;
    extern PFNGLGETPATHDASHARRAYNVPROC                             glGetPathDashArrayNV;
    extern PFNGLGETPATHLENGTHNVPROC                                glGetPathLengthNV;
    extern PFNGLGETPATHMETRICRANGENVPROC                           glGetPathMetricRangeNV;
    extern PFNGLGETPATHMETRICSNVPROC                               glGetPathMetricsNV;
    extern PFNGLGETPATHPARAMETERFVNVPROC                           glGetPathParameterfvNV;
    extern PFNGLGETPATHPARAMETERIVNVPROC                           glGetPathParameterivNV;
    extern PFNGLGETPATHSPACINGNVPROC                               glGetPathSpacingNV;
    extern PFNGLGETPERFCOUNTERINFOINTELPROC                        glGetPerfCounterInfoINTEL;
    extern PFNGLGETPERFMONITORCOUNTERDATAAMDPROC                   glGetPerfMonitorCounterDataAMD;
    extern PFNGLGETPERFMONITORCOUNTERINFOAMDPROC                   glGetPerfMonitorCounterInfoAMD;
    extern PFNGLGETPERFMONITORCOUNTERSTRINGAMDPROC                 glGetPerfMonitorCounterStringAMD;
    extern PFNGLGETPERFMONITORCOUNTERSAMDPROC                      glGetPerfMonitorCountersAMD;
    extern PFNGLGETPERFMONITORGROUPSTRINGAMDPROC                   glGetPerfMonitorGroupStringAMD;
    extern PFNGLGETPERFMONITORGROUPSAMDPROC                        glGetPerfMonitorGroupsAMD;
    extern PFNGLGETPERFQUERYDATAINTELPROC                          glGetPerfQueryDataINTEL;
    extern PFNGLGETPERFQUERYIDBYNAMEINTELPROC                      glGetPerfQueryIdByNameINTEL;
    extern PFNGLGETPERFQUERYINFOINTELPROC                          glGetPerfQueryInfoINTEL;
    extern PFNGLGETPOINTERVKHRPROC                                 glGetPointervKHR;
    extern PFNGLGETPROGRAMBINARYOESPROC                            glGetProgramBinaryOES;
    extern PFNGLGETPROGRAMPIPELINEINFOLOGEXTPROC                   glGetProgramPipelineInfoLogEXT;
    extern PFNGLGETPROGRAMPIPELINEIVEXTPROC                        glGetProgramPipelineivEXT;
    extern PFNGLGETPROGRAMRESOURCELOCATIONINDEXEXTPROC             glGetProgramResourceLocationIndexEXT;
    extern PFNGLGETPROGRAMRESOURCEFVNVPROC                         glGetProgramResourcefvNV;
    extern PFNGLGETQUERYOBJECTI64VEXTPROC                          glGetQueryObjecti64vEXT;
    extern PFNGLGETQUERYOBJECTIVEXTPROC                            glGetQueryObjectivEXT;
    extern PFNGLGETQUERYOBJECTUI64VEXTPROC                         glGetQueryObjectui64vEXT;
    extern PFNGLGETQUERYOBJECTUIVEXTPROC                           glGetQueryObjectuivEXT;
    extern PFNGLGETQUERYIVEXTPROC                                  glGetQueryivEXT;
    extern PFNGLGETSAMPLERPARAMETERIIVEXTPROC                      glGetSamplerParameterIivEXT;
    extern PFNGLGETSAMPLERPARAMETERIIVOESPROC                      glGetSamplerParameterIivOES;
    extern PFNGLGETSAMPLERPARAMETERIUIVEXTPROC                     glGetSamplerParameterIuivEXT;
    extern PFNGLGETSAMPLERPARAMETERIUIVOESPROC                     glGetSamplerParameterIuivOES;
    extern PFNGLGETSYNCIVAPPLEPROC                                 glGetSyncivAPPLE;
    extern PFNGLGETTEXPARAMETERIIVEXTPROC                          glGetTexParameterIivEXT;
    extern PFNGLGETTEXPARAMETERIIVOESPROC                          glGetTexParameterIivOES;
    extern PFNGLGETTEXPARAMETERIUIVEXTPROC                         glGetTexParameterIuivEXT;
    extern PFNGLGETTEXPARAMETERIUIVOESPROC                         glGetTexParameterIuivOES;
    extern PFNGLGETTEXTUREHANDLENVPROC                             glGetTextureHandleNV;
    extern PFNGLGETTEXTURESAMPLERHANDLENVPROC                      glGetTextureSamplerHandleNV;
    extern PFNGLGETTRANSLATEDSHADERSOURCEANGLEPROC                 glGetTranslatedShaderSourceANGLE;
    extern PFNGLGETNUNIFORMFVEXTPROC                               glGetnUniformfvEXT;
    extern PFNGLGETNUNIFORMFVKHRPROC                               glGetnUniformfvKHR;
    extern PFNGLGETNUNIFORMIVEXTPROC                               glGetnUniformivEXT;
    extern PFNGLGETNUNIFORMIVKHRPROC                               glGetnUniformivKHR;
    extern PFNGLGETNUNIFORMUIVKHRPROC                              glGetnUniformuivKHR;
    extern PFNGLINSERTEVENTMARKEREXTPROC                           glInsertEventMarkerEXT;
    extern PFNGLINTERPOLATEPATHSNVPROC                             glInterpolatePathsNV;
    extern PFNGLISENABLEDIEXTPROC                                  glIsEnablediEXT;
    extern PFNGLISENABLEDINVPROC                                   glIsEnablediNV;
    extern PFNGLISENABLEDIOESPROC                                  glIsEnablediOES;
    extern PFNGLISFENCENVPROC                                      glIsFenceNV;
    extern PFNGLISIMAGEHANDLERESIDENTNVPROC                        glIsImageHandleResidentNV;
    extern PFNGLISPATHNVPROC                                       glIsPathNV;
    extern PFNGLISPOINTINFILLPATHNVPROC                            glIsPointInFillPathNV;
    extern PFNGLISPOINTINSTROKEPATHNVPROC                          glIsPointInStrokePathNV;
    extern PFNGLISPROGRAMPIPELINEEXTPROC                           glIsProgramPipelineEXT;
    extern PFNGLISQUERYEXTPROC                                     glIsQueryEXT;
    extern PFNGLISSYNCAPPLEPROC                                    glIsSyncAPPLE;
    extern PFNGLISTEXTUREHANDLERESIDENTNVPROC                      glIsTextureHandleResidentNV;
    extern PFNGLISVERTEXARRAYOESPROC                               glIsVertexArrayOES;
    extern PFNGLLABELOBJECTEXTPROC                                 glLabelObjectEXT;
    extern PFNGLMAKEIMAGEHANDLENONRESIDENTNVPROC                   glMakeImageHandleNonResidentNV;
    extern PFNGLMAKEIMAGEHANDLERESIDENTNVPROC                      glMakeImageHandleResidentNV;
    extern PFNGLMAKETEXTUREHANDLENONRESIDENTNVPROC                 glMakeTextureHandleNonResidentNV;
    extern PFNGLMAKETEXTUREHANDLERESIDENTNVPROC                    glMakeTextureHandleResidentNV;
    extern PFNGLMAPBUFFEROESPROC                                   glMapBufferOES;
    extern PFNGLMAPBUFFERRANGEEXTPROC                              glMapBufferRangeEXT;
    extern PFNGLMATRIXLOAD3X2FNVPROC                               glMatrixLoad3x2fNV;
    extern PFNGLMATRIXLOAD3X3FNVPROC                               glMatrixLoad3x3fNV;
    extern PFNGLMATRIXLOADTRANSPOSE3X3FNVPROC                      glMatrixLoadTranspose3x3fNV;
    extern PFNGLMATRIXMULT3X2FNVPROC                               glMatrixMult3x2fNV;
    extern PFNGLMATRIXMULT3X3FNVPROC                               glMatrixMult3x3fNV;
    extern PFNGLMATRIXMULTTRANSPOSE3X3FNVPROC                      glMatrixMultTranspose3x3fNV;
    extern PFNGLMINSAMPLESHADINGOESPROC                            glMinSampleShadingOES;
    extern PFNGLMULTIDRAWARRAYSEXTPROC                             glMultiDrawArraysEXT;
    extern PFNGLMULTIDRAWARRAYSINDIRECTEXTPROC                     glMultiDrawArraysIndirectEXT;
    extern PFNGLMULTIDRAWELEMENTSBASEVERTEXEXTPROC                 glMultiDrawElementsBaseVertexEXT;
    extern PFNGLMULTIDRAWELEMENTSBASEVERTEXOESPROC                 glMultiDrawElementsBaseVertexOES;
    extern PFNGLMULTIDRAWELEMENTSEXTPROC                           glMultiDrawElementsEXT;
    extern PFNGLMULTIDRAWELEMENTSINDIRECTEXTPROC                   glMultiDrawElementsIndirectEXT;
    extern PFNGLNAMEDFRAMEBUFFERSAMPLELOCATIONSFVNVPROC            glNamedFramebufferSampleLocationsfvNV;
    extern PFNGLOBJECTLABELKHRPROC                                 glObjectLabelKHR;
    extern PFNGLOBJECTPTRLABELKHRPROC                              glObjectPtrLabelKHR;
    extern PFNGLPATCHPARAMETERIEXTPROC                             glPatchParameteriEXT;
    extern PFNGLPATCHPARAMETERIOESPROC                             glPatchParameteriOES;
    extern PFNGLPATHCOMMANDSNVPROC                                 glPathCommandsNV;
    extern PFNGLPATHCOORDSNVPROC                                   glPathCoordsNV;
    extern PFNGLPATHCOVERDEPTHFUNCNVPROC                           glPathCoverDepthFuncNV;
    extern PFNGLPATHDASHARRAYNVPROC                                glPathDashArrayNV;
    extern PFNGLPATHGLYPHINDEXARRAYNVPROC                          glPathGlyphIndexArrayNV;
    extern PFNGLPATHGLYPHINDEXRANGENVPROC                          glPathGlyphIndexRangeNV;
    extern PFNGLPATHGLYPHRANGENVPROC                               glPathGlyphRangeNV;
    extern PFNGLPATHGLYPHSNVPROC                                   glPathGlyphsNV;
    extern PFNGLPATHMEMORYGLYPHINDEXARRAYNVPROC                    glPathMemoryGlyphIndexArrayNV;
    extern PFNGLPATHPARAMETERFNVPROC                               glPathParameterfNV;
    extern PFNGLPATHPARAMETERFVNVPROC                              glPathParameterfvNV;
    extern PFNGLPATHPARAMETERINVPROC                               glPathParameteriNV;
    extern PFNGLPATHPARAMETERIVNVPROC                              glPathParameterivNV;
    extern PFNGLPATHSTENCILDEPTHOFFSETNVPROC                       glPathStencilDepthOffsetNV;
    extern PFNGLPATHSTENCILFUNCNVPROC                              glPathStencilFuncNV;
    extern PFNGLPATHSTRINGNVPROC                                   glPathStringNV;
    extern PFNGLPATHSUBCOMMANDSNVPROC                              glPathSubCommandsNV;
    extern PFNGLPATHSUBCOORDSNVPROC                                glPathSubCoordsNV;
    extern PFNGLPOINTALONGPATHNVPROC                               glPointAlongPathNV;
    extern PFNGLPOLYGONMODENVPROC                                  glPolygonModeNV;
    extern PFNGLPOLYGONOFFSETCLAMPEXTPROC                          glPolygonOffsetClampEXT;
    extern PFNGLPOPDEBUGGROUPKHRPROC                               glPopDebugGroupKHR;
    extern PFNGLPOPGROUPMARKEREXTPROC                              glPopGroupMarkerEXT;
    extern PFNGLPRIMITIVEBOUNDINGBOXEXTPROC                        glPrimitiveBoundingBoxEXT;
    extern PFNGLPRIMITIVEBOUNDINGBOXOESPROC                        glPrimitiveBoundingBoxOES;
    extern PFNGLPROGRAMBINARYOESPROC                               glProgramBinaryOES;
    extern PFNGLPROGRAMPARAMETERIEXTPROC                           glProgramParameteriEXT;
    extern PFNGLPROGRAMPATHFRAGMENTINPUTGENNVPROC                  glProgramPathFragmentInputGenNV;
    extern PFNGLPROGRAMUNIFORM1FEXTPROC                            glProgramUniform1fEXT;
    extern PFNGLPROGRAMUNIFORM1FVEXTPROC                           glProgramUniform1fvEXT;
    extern PFNGLPROGRAMUNIFORM1IEXTPROC                            glProgramUniform1iEXT;
    extern PFNGLPROGRAMUNIFORM1IVEXTPROC                           glProgramUniform1ivEXT;
    extern PFNGLPROGRAMUNIFORM1UIEXTPROC                           glProgramUniform1uiEXT;
    extern PFNGLPROGRAMUNIFORM1UIVEXTPROC                          glProgramUniform1uivEXT;
    extern PFNGLPROGRAMUNIFORM2FEXTPROC                            glProgramUniform2fEXT;
    extern PFNGLPROGRAMUNIFORM2FVEXTPROC                           glProgramUniform2fvEXT;
    extern PFNGLPROGRAMUNIFORM2IEXTPROC                            glProgramUniform2iEXT;
    extern PFNGLPROGRAMUNIFORM2IVEXTPROC                           glProgramUniform2ivEXT;
    extern PFNGLPROGRAMUNIFORM2UIEXTPROC                           glProgramUniform2uiEXT;
    extern PFNGLPROGRAMUNIFORM2UIVEXTPROC                          glProgramUniform2uivEXT;
    extern PFNGLPROGRAMUNIFORM3FEXTPROC                            glProgramUniform3fEXT;
    extern PFNGLPROGRAMUNIFORM3FVEXTPROC                           glProgramUniform3fvEXT;
    extern PFNGLPROGRAMUNIFORM3IEXTPROC                            glProgramUniform3iEXT;
    extern PFNGLPROGRAMUNIFORM3IVEXTPROC                           glProgramUniform3ivEXT;
    extern PFNGLPROGRAMUNIFORM3UIEXTPROC                           glProgramUniform3uiEXT;
    extern PFNGLPROGRAMUNIFORM3UIVEXTPROC                          glProgramUniform3uivEXT;
    extern PFNGLPROGRAMUNIFORM4FEXTPROC                            glProgramUniform4fEXT;
    extern PFNGLPROGRAMUNIFORM4FVEXTPROC                           glProgramUniform4fvEXT;
    extern PFNGLPROGRAMUNIFORM4IEXTPROC                            glProgramUniform4iEXT;
    extern PFNGLPROGRAMUNIFORM4IVEXTPROC                           glProgramUniform4ivEXT;
    extern PFNGLPROGRAMUNIFORM4UIEXTPROC                           glProgramUniform4uiEXT;
    extern PFNGLPROGRAMUNIFORM4UIVEXTPROC                          glProgramUniform4uivEXT;
    extern PFNGLPROGRAMUNIFORMHANDLEUI64NVPROC                     glProgramUniformHandleui64NV;
    extern PFNGLPROGRAMUNIFORMHANDLEUI64VNVPROC                    glProgramUniformHandleui64vNV;
    extern PFNGLPROGRAMUNIFORMMATRIX2FVEXTPROC                     glProgramUniformMatrix2fvEXT;
    extern PFNGLPROGRAMUNIFORMMATRIX2X3FVEXTPROC                   glProgramUniformMatrix2x3fvEXT;
    extern PFNGLPROGRAMUNIFORMMATRIX2X4FVEXTPROC                   glProgramUniformMatrix2x4fvEXT;
    extern PFNGLPROGRAMUNIFORMMATRIX3FVEXTPROC                     glProgramUniformMatrix3fvEXT;
    extern PFNGLPROGRAMUNIFORMMATRIX3X2FVEXTPROC                   glProgramUniformMatrix3x2fvEXT;
    extern PFNGLPROGRAMUNIFORMMATRIX3X4FVEXTPROC                   glProgramUniformMatrix3x4fvEXT;
    extern PFNGLPROGRAMUNIFORMMATRIX4FVEXTPROC                     glProgramUniformMatrix4fvEXT;
    extern PFNGLPROGRAMUNIFORMMATRIX4X2FVEXTPROC                   glProgramUniformMatrix4x2fvEXT;
    extern PFNGLPROGRAMUNIFORMMATRIX4X3FVEXTPROC                   glProgramUniformMatrix4x3fvEXT;
    extern PFNGLPUSHDEBUGGROUPKHRPROC                              glPushDebugGroupKHR;
    extern PFNGLPUSHGROUPMARKEREXTPROC                             glPushGroupMarkerEXT;
    extern PFNGLQUERYCOUNTEREXTPROC                                glQueryCounterEXT;
    extern PFNGLRASTERSAMPLESEXTPROC                               glRasterSamplesEXT;
    extern PFNGLREADBUFFERINDEXEDEXTPROC                           glReadBufferIndexedEXT;
    extern PFNGLREADBUFFERNVPROC                                   glReadBufferNV;
    extern PFNGLREADNPIXELSEXTPROC                                 glReadnPixelsEXT;
    extern PFNGLREADNPIXELSKHRPROC                                 glReadnPixelsKHR;
    extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLEANGLEPROC            glRenderbufferStorageMultisampleANGLE;
    extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLEAPPLEPROC            glRenderbufferStorageMultisampleAPPLE;
    extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC              glRenderbufferStorageMultisampleEXT;
    extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMGPROC              glRenderbufferStorageMultisampleIMG;
    extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLENVPROC               glRenderbufferStorageMultisampleNV;
    extern PFNGLRESOLVEDEPTHVALUESNVPROC                           glResolveDepthValuesNV;
    extern PFNGLRESOLVEMULTISAMPLEFRAMEBUFFERAPPLEPROC             glResolveMultisampleFramebufferAPPLE;
    extern PFNGLSAMPLERPARAMETERIIVEXTPROC                         glSamplerParameterIivEXT;
    extern PFNGLSAMPLERPARAMETERIIVOESPROC                         glSamplerParameterIivOES;
    extern PFNGLSAMPLERPARAMETERIUIVEXTPROC                        glSamplerParameterIuivEXT;
    extern PFNGLSAMPLERPARAMETERIUIVOESPROC                        glSamplerParameterIuivOES;
    extern PFNGLSCISSORARRAYVNVPROC                                glScissorArrayvNV;
    extern PFNGLSCISSORINDEXEDNVPROC                               glScissorIndexedNV;
    extern PFNGLSCISSORINDEXEDVNVPROC                              glScissorIndexedvNV;
    extern PFNGLSELECTPERFMONITORCOUNTERSAMDPROC                   glSelectPerfMonitorCountersAMD;
    extern PFNGLSETFENCENVPROC                                     glSetFenceNV;
    extern PFNGLSTARTTILINGQCOMPROC                                glStartTilingQCOM;
    extern PFNGLSTENCILFILLPATHINSTANCEDNVPROC                     glStencilFillPathInstancedNV;
    extern PFNGLSTENCILFILLPATHNVPROC                              glStencilFillPathNV;
    extern PFNGLSTENCILSTROKEPATHINSTANCEDNVPROC                   glStencilStrokePathInstancedNV;
    extern PFNGLSTENCILSTROKEPATHNVPROC                            glStencilStrokePathNV;
    extern PFNGLSTENCILTHENCOVERFILLPATHINSTANCEDNVPROC            glStencilThenCoverFillPathInstancedNV;
    extern PFNGLSTENCILTHENCOVERFILLPATHNVPROC                     glStencilThenCoverFillPathNV;
    extern PFNGLSTENCILTHENCOVERSTROKEPATHINSTANCEDNVPROC          glStencilThenCoverStrokePathInstancedNV;
    extern PFNGLSTENCILTHENCOVERSTROKEPATHNVPROC                   glStencilThenCoverStrokePathNV;
    extern PFNGLSUBPIXELPRECISIONBIASNVPROC                        glSubpixelPrecisionBiasNV;
    extern PFNGLTESTFENCENVPROC                                    glTestFenceNV;
    extern PFNGLTEXBUFFEREXTPROC                                   glTexBufferEXT;
    extern PFNGLTEXBUFFEROESPROC                                   glTexBufferOES;
    extern PFNGLTEXBUFFERRANGEEXTPROC                              glTexBufferRangeEXT;
    extern PFNGLTEXBUFFERRANGEOESPROC                              glTexBufferRangeOES;
    extern PFNGLTEXIMAGE3DOESPROC                                  glTexImage3DOES;
    extern PFNGLTEXPAGECOMMITMENTEXTPROC                           glTexPageCommitmentEXT;
    extern PFNGLTEXPARAMETERIIVEXTPROC                             glTexParameterIivEXT;
    extern PFNGLTEXPARAMETERIIVOESPROC                             glTexParameterIivOES;
    extern PFNGLTEXPARAMETERIUIVEXTPROC                            glTexParameterIuivEXT;
    extern PFNGLTEXPARAMETERIUIVOESPROC                            glTexParameterIuivOES;
    extern PFNGLTEXSTORAGE1DEXTPROC                                glTexStorage1DEXT;
    extern PFNGLTEXSTORAGE2DEXTPROC                                glTexStorage2DEXT;
    extern PFNGLTEXSTORAGE3DEXTPROC                                glTexStorage3DEXT;
    extern PFNGLTEXSTORAGE3DMULTISAMPLEOESPROC                     glTexStorage3DMultisampleOES;
    extern PFNGLTEXSUBIMAGE3DOESPROC                               glTexSubImage3DOES;
    extern PFNGLTEXTURESTORAGE1DEXTPROC                            glTextureStorage1DEXT;
    extern PFNGLTEXTURESTORAGE2DEXTPROC                            glTextureStorage2DEXT;
    extern PFNGLTEXTURESTORAGE3DEXTPROC                            glTextureStorage3DEXT;
    extern PFNGLTEXTUREVIEWEXTPROC                                 glTextureViewEXT;
    extern PFNGLTEXTUREVIEWOESPROC                                 glTextureViewOES;
    extern PFNGLTRANSFORMPATHNVPROC                                glTransformPathNV;
    extern PFNGLUNIFORMHANDLEUI64NVPROC                            glUniformHandleui64NV;
    extern PFNGLUNIFORMHANDLEUI64VNVPROC                           glUniformHandleui64vNV;
    extern PFNGLUNIFORMMATRIX2X3FVNVPROC                           glUniformMatrix2x3fvNV;
    extern PFNGLUNIFORMMATRIX2X4FVNVPROC                           glUniformMatrix2x4fvNV;
    extern PFNGLUNIFORMMATRIX3X2FVNVPROC                           glUniformMatrix3x2fvNV;
    extern PFNGLUNIFORMMATRIX3X4FVNVPROC                           glUniformMatrix3x4fvNV;
    extern PFNGLUNIFORMMATRIX4X2FVNVPROC                           glUniformMatrix4x2fvNV;
    extern PFNGLUNIFORMMATRIX4X3FVNVPROC                           glUniformMatrix4x3fvNV;
    extern PFNGLUNMAPBUFFEROESPROC                                 glUnmapBufferOES;
    extern PFNGLUSEPROGRAMSTAGESEXTPROC                            glUseProgramStagesEXT;
    extern PFNGLVALIDATEPROGRAMPIPELINEEXTPROC                     glValidateProgramPipelineEXT;
    extern PFNGLVERTEXATTRIBDIVISORANGLEPROC                       glVertexAttribDivisorANGLE;
    extern PFNGLVERTEXATTRIBDIVISOREXTPROC                         glVertexAttribDivisorEXT;
    extern PFNGLVERTEXATTRIBDIVISORNVPROC                          glVertexAttribDivisorNV;
    extern PFNGLVIEWPORTARRAYVNVPROC                               glViewportArrayvNV;
    extern PFNGLVIEWPORTINDEXEDFNVPROC                             glViewportIndexedfNV;
    extern PFNGLVIEWPORTINDEXEDFVNVPROC                            glViewportIndexedfvNV;
    extern PFNGLWAITSYNCAPPLEPROC                                  glWaitSyncAPPLE;
    extern PFNGLWEIGHTPATHSNVPROC                                  glWeightPathsNV;
#endif
    
#ifdef __cplusplus
}
#endif

#endif

#endif

//
// Copyright (c) 2017 The Khronos Group Inc.
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "testBase.h"
#include "helpers.h"

#include "gl_headers.h"

extern "C" { extern cl_uint gRandomSeed; };

extern int test_cl_image_write( cl_context context, cl_command_queue queue, cl_mem clImage,
                                size_t imageWidth, size_t imageHeight, cl_image_format *outFormat,
                                ExplicitType *outType, void **outSourceBuffer, MTdata d );

extern int test_cl_image_read( cl_context context, cl_command_queue queue, cl_mem clImage,
                               size_t imageWidth, size_t imageHeight, cl_image_format *outFormat,
                               ExplicitType *outType, void **outResultBuffer );

static int test_attach_renderbuffer_read_image( cl_context context, cl_command_queue queue, GLenum glTarget, GLuint glRenderbuffer,
                    size_t imageWidth, size_t imageHeight, cl_image_format *outFormat, ExplicitType *outType, void **outResultBuffer )
{
    int error;

    // Create a CL image from the supplied GL renderbuffer
    clMemWrapper image = (*clCreateFromGLRenderbuffer_ptr)( context, CL_MEM_READ_ONLY, glRenderbuffer, &error );
    if( error != CL_SUCCESS )
    {
        print_error( error, "Unable to create CL image from GL renderbuffer" );
        return error;
    }

    return test_cl_image_read( context, queue, image, imageWidth, imageHeight, outFormat, outType, outResultBuffer );
}

int test_renderbuffer_read_image( cl_context context, cl_command_queue queue,
                            GLsizei width, GLsizei height, GLenum attachment,
                            GLenum rbFormat, GLenum rbType,
                            GLenum texFormat, GLenum texType,
                            ExplicitType type, MTdata d )
{
    int error;


    // Create the GL renderbuffer
    glFramebufferWrapper glFramebuffer;
    glRenderbufferWrapper glRenderbuffer;
    void *tmp = CreateGLRenderbuffer( width, height, attachment, rbFormat, rbType, texFormat, texType,
                                     type, &glFramebuffer, &glRenderbuffer, &error, d, true );
    BufferOwningPtr<char> inputBuffer(tmp);
    if( error != 0 )
    {
        // GL_RGBA_INTEGER_EXT doesn't exist in GLES2. No need to check for it.
        return error;
    }

    // Run and get the results
    cl_image_format clFormat;
    ExplicitType actualType;
    char *outBuffer;
    error = test_attach_renderbuffer_read_image( context, queue, attachment, glRenderbuffer, width, height, &clFormat, &actualType, (void **)&outBuffer );
    if( error != 0 )
        return error;
    BufferOwningPtr<char> actualResults(outBuffer);

    log_info( "- Read [%4d x %4d] : GL renderbuffer : %s : %s : %s => CL Image : %s : %s \n", width, height,
                    GetGLFormatName( rbFormat ), GetGLFormatName( rbFormat ), GetGLTypeName( rbType ),
                    GetChannelOrderName( clFormat.image_channel_order ), GetChannelTypeName( clFormat.image_channel_data_type ));

#ifdef GLES_DEBUG
    log_info("- start read GL data -- \n");
    DumpGLBuffer(glType, width, height, actualResults);
    log_info("- end read GL data -- \n");
#endif

    // We have to convert our input buffer to the returned type, so we can validate.
    BufferOwningPtr<char> convertedInput(convert_to_expected( inputBuffer, width * height, type, actualType ));

#ifdef GLES_DEBUG
    log_info("- start input data -- \n");
    DumpGLBuffer(GetGLTypeForExplicitType(actualType), width, height, convertedInput);
    log_info("- end input data -- \n");
#endif

#ifdef GLES_DEBUG
    log_info("- start converted data -- \n");
    DumpGLBuffer(GetGLTypeForExplicitType(actualType), width, height, actualResults);
    log_info("- end converted data -- \n");
#endif

    // Now we validate
    int valid = 0;
    if(convertedInput) {
        if( actualType == kFloat )
            valid = validate_float_results( convertedInput, actualResults, width, height );
        else
            valid = validate_integer_results( convertedInput, actualResults, width, height, get_explicit_type_size( actualType ) );
    }

    return valid;
}

int test_renderbuffer_read( cl_device_id device, cl_context context, cl_command_queue queue, int numElements )
{
    GLenum attachments[] = { GL_COLOR_ATTACHMENT0_EXT };

    struct {
        GLenum rbFormat;
        GLenum rbType;
        GLenum texFormat;
        GLenum texType;
        ExplicitType type;

    } formats[] = {
        { GL_RGBA8_OES,    GL_UNSIGNED_BYTE,   GL_RGBA,           GL_UNSIGNED_BYTE,            kUChar },
        //{ GL_RGBA16F_QCOM, GL_HALF_FLOAT_OES,  GL_RGBA,           GL_HALF_FLOAT_OES,           kHalf  },  // Half-float not supported by ReadPixels
        { GL_RGBA32F,      GL_FLOAT,           GL_RGBA,           GL_FLOAT,                    kFloat},
        // XXX add others
    };

    size_t fmtIdx, attIdx;
    int error = 0;
#ifdef GLES_DEBUG
    size_t iter = 1;
#else
    size_t iter = 6;
#endif
    RandomSeed seed( gRandomSeed );

    // Check if images are supported
  if (checkForImageSupport(device)) {
    log_info("Device does not support images. Skipping test.\n");
    return 0;
  }

    // Loop through a set of GL formats, testing a set of sizes against each one
    for( fmtIdx = 0; fmtIdx < sizeof( formats ) / sizeof( formats[ 0 ] ); fmtIdx++ )
    {
        for( attIdx = 0; attIdx < sizeof( attachments ) / sizeof( attachments[ 0 ] ); attIdx++ )
        {
            size_t i;

            log_info( "Testing renderbuffer read for %s : %s : %s : %s\n",
                GetGLAttachmentName( attachments[ attIdx ] ),
                GetGLFormatName( formats[ fmtIdx ].rbFormat ),
                GetGLBaseFormatName( formats[ fmtIdx ].rbFormat ),
                GetGLTypeName( formats[ fmtIdx ].rbType ) );

            for( i = 0; i < iter; i++ )
            {
                GLsizei width = random_in_range( 16, 512, seed );
                GLsizei height = random_in_range( 16, 512, seed );
#ifdef GLES_DEBUG
                width = height = 4;
#endif

                if( test_renderbuffer_read_image( context, queue, width, height,
                                                  attachments[ attIdx ],
                                                  formats[ fmtIdx ].rbFormat,
                                                  formats[ fmtIdx ].rbType,
                                                  formats[ fmtIdx ].texFormat,
                                                  formats[ fmtIdx ].texType,
                                                  formats[ fmtIdx ].type, seed ) )

                {
                    log_error( "ERROR: Renderbuffer read test failed for %s : %s : %s : %s\n\n",
                                GetGLAttachmentName( attachments[ attIdx ] ),
                                GetGLFormatName( formats[ fmtIdx ].rbFormat ),
                                GetGLBaseFormatName( formats[ fmtIdx ].rbFormat ),
                                GetGLTypeName( formats[ fmtIdx ].rbType ) );

                    error++;
                    break;    // Skip other sizes for this combination
                }
            }
            if( i == iter )
            {
                log_info( "passed: Renderbuffer read test passed for %s : %s : %s : %s\n\n",
                          GetGLAttachmentName( attachments[ attIdx ] ),
                          GetGLFormatName( formats[ fmtIdx ].rbFormat ),
                          GetGLBaseFormatName( formats[ fmtIdx ].rbFormat ),
                          GetGLTypeName( formats[ fmtIdx ].rbType ) );
            }
        }
    }

    return error;
}


#pragma mark -------------------- Write tests -------------------------

int test_attach_renderbuffer_write_to_image( cl_context context, cl_command_queue queue, GLenum glTarget, GLuint glRenderbuffer,
                     size_t imageWidth, size_t imageHeight, cl_image_format *outFormat, ExplicitType *outType, MTdata d, void **outSourceBuffer )
{
    int error;

    // Create a CL image from the supplied GL renderbuffer
    clMemWrapper image = (*clCreateFromGLRenderbuffer_ptr)( context, CL_MEM_WRITE_ONLY, glRenderbuffer, &error );
    if( error != CL_SUCCESS )
    {
        print_error( error, "Unable to create CL image from GL renderbuffer" );
        return error;
    }

    return test_cl_image_write( context, queue, image, imageWidth, imageHeight, outFormat, outType, outSourceBuffer, d );
}

int test_renderbuffer_image_write( cl_context context, cl_command_queue queue,
                                   GLsizei width, GLsizei height, GLenum attachment,
                                      GLenum rbFormat, GLenum rbType,
                                   GLenum texFormat, GLenum texType,
                                     ExplicitType type, MTdata d)
{
    int error;

    // Create the GL renderbuffer
    glFramebufferWrapper glFramebuffer;
    glRenderbufferWrapper glRenderbuffer;
    CreateGLRenderbuffer( width, height, attachment, rbFormat, rbType, texFormat, texType,
                         type, &glFramebuffer, &glRenderbuffer, &error, d, false );
    if( error != 0 )
    {
        // GL_RGBA_INTEGER_EXT doesn't exist in GLES2. No need to check for it.
        return error;
    }

    // Run and get the results
    cl_image_format clFormat;
    ExplicitType sourceType;
    void *outSourceBuffer;
    error = test_attach_renderbuffer_write_to_image( context, queue, attachment, glRenderbuffer, width, height, &clFormat, &sourceType, d, (void **)&outSourceBuffer );
    if( error != 0 )
        return error;

    BufferOwningPtr<char> sourceData(outSourceBuffer);

    log_info( "- Write [%4d x %4d] : GL Renderbuffer : %s : %s : %s => CL Image : %s : %s \n", width, height,
                    GetGLFormatName( rbFormat ), GetGLFormatName( rbFormat ), GetGLTypeName( rbType),
                    GetChannelOrderName( clFormat.image_channel_order ), GetChannelTypeName( clFormat.image_channel_data_type ));

    // Now read the results from the GL renderbuffer
    void* tmp = ReadGLRenderbuffer( glFramebuffer, glRenderbuffer, attachment, rbFormat, rbType,
                                    texFormat, texType, type, width, height );
    BufferOwningPtr<char> resultData( tmp );

#ifdef GLES_DEBUG
    log_info("- start result data -- \n");
    DumpGLBuffer(glType, width, height, resultData);
    log_info("- end result data -- \n");
#endif

    // We have to convert our input buffer to the returned type, so we can validate.
    BufferOwningPtr<char> convertedData( convert_to_expected( resultData, width * height, type, sourceType ) );

#ifdef GLES_DEBUG
    log_info("- start input data -- \n");
    DumpGLBuffer(GetGLTypeForExplicitType(sourceType), width, height, sourceData);
    log_info("- end input data -- \n");
#endif

#ifdef GLES_DEBUG
    log_info("- start converted data -- \n");
    DumpGLBuffer(GetGLTypeForExplicitType(sourceType), width, height, convertedData);
    log_info("- end converted data -- \n");
#endif

    // Now we validate
    int valid = 0;
    if(convertedData) {
        if( sourceType == kFloat )
            valid = validate_float_results( sourceData, convertedData, width, height );
        else
            valid = validate_integer_results( sourceData, convertedData, width, height, get_explicit_type_size( type ) );
    }

    return valid;
}

int test_renderbuffer_write( cl_device_id device, cl_context context, cl_command_queue queue, int numElements )
{
    GLenum attachments[] = { GL_COLOR_ATTACHMENT0_EXT };

    struct {
        GLenum rbFormat;
        GLenum rbType;
        GLenum texFormat;
        GLenum texType;
        ExplicitType type;

    } formats[] = {
        { GL_RGBA8_OES,    GL_UNSIGNED_BYTE,   GL_RGBA,           GL_UNSIGNED_BYTE,            kUChar },
        //{ GL_RGBA16F_QCOM, GL_UNSIGNED_SHORT,  GL_RGBA,           GL_UNSIGNED_SHORT,           kHalf  },  // Half float not supported by ReadPixels
        { GL_RGBA32F,      GL_FLOAT,           GL_RGBA,           GL_FLOAT,                    kFloat},
        // XXX add others
    };

    size_t fmtIdx, attIdx;
    int error = 0;
    size_t iter = 6;
#ifdef GLES_DEBUG
    iter = 1;
#endif
    RandomSeed seed( gRandomSeed );

    // Check if images are supported
  if (checkForImageSupport(device)) {
    log_info("Device does not support images. Skipping test.\n");
    return 0;
  }

    // Loop through a set of GL formats, testing a set of sizes against each one
    for( fmtIdx = 0; fmtIdx < sizeof( formats ) / sizeof( formats[ 0 ] ); fmtIdx++ )
    {
        for( attIdx = 0; attIdx < sizeof( attachments ) / sizeof( attachments[ 0 ] ); attIdx++ )
        {
            log_info( "Testing Renderbuffer write test for %s : %s : %s : %s\n",
                GetGLAttachmentName( attachments[ attIdx ] ),
                GetGLFormatName( formats[ fmtIdx ].rbFormat ),
                GetGLBaseFormatName( formats[ fmtIdx ].rbFormat ),
                GetGLTypeName( formats[ fmtIdx ].rbType) );

            size_t i;
            for( i = 0; i < iter; i++ )
            {
                GLsizei width = random_in_range( 16, 512, seed );
                GLsizei height = random_in_range( 16, 512, seed );
#ifdef GLES_DEBUG
                width = height = 4;
#endif

                if( test_renderbuffer_image_write( context, queue, width, height,
                                                   attachments[ attIdx ],
                                                   formats[ fmtIdx ].rbFormat,
                                                   formats[ fmtIdx ].rbType,
                                                   formats[ fmtIdx ].texFormat,
                                                   formats[ fmtIdx ].texType,
                                                   formats[ fmtIdx ].type, seed ) )
                {
                    log_error( "ERROR: Renderbuffer write test failed for %s : %s : %s : %s\n\n",
                          GetGLAttachmentName( attachments[ attIdx ] ),
                          GetGLFormatName( formats[ fmtIdx ].rbFormat ),
                          GetGLBaseFormatName( formats[ fmtIdx ].rbFormat ),
                          GetGLTypeName( formats[ fmtIdx ].rbType ) );

                    error++;
                    break;    // Skip other sizes for this combination
                }
            }
            if( i == iter )
            {
                log_info( "passed: Renderbuffer write test passed for %s : %s : %s : %s\n\n",
                          GetGLAttachmentName( attachments[ attIdx ] ),
                          GetGLFormatName( formats[ fmtIdx ].rbFormat ),
                          GetGLBaseFormatName( formats[ fmtIdx ].rbFormat ),
                          GetGLTypeName( formats[ fmtIdx ].rbType ) );
            }
        }
    }

    return error;
}
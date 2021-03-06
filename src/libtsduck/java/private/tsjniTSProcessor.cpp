//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2021, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  Native implementation of the Java class io.tsduck.TSProcessor.
//
//----------------------------------------------------------------------------

#include "tsTSProcessor.h"
#include "tsNullReport.h"
#include "tsjni.h"
TSDUCK_SOURCE;

#if !defined(TS_NO_JAVA)

//----------------------------------------------------------------------------
// Interface of native methods.
//----------------------------------------------------------------------------

extern "C" {
    // Method: io.tsduck.TSProcessor.initNativeObject
    // Signature: (Lio/tsduck/Report;)V
    JNIEXPORT void JNICALL Java_io_tsduck_TSProcessor_initNativeObject(JNIEnv*, jobject, jobject);

    // Method: io.tsduck.TSProcessor.start
    // Signature: ()B
    JNIEXPORT jboolean JNICALL Java_io_tsduck_TSProcessor_start(JNIEnv*, jobject);

    // Method: io.tsduck.TSProcessor.abort
    // Signature: ()V
    JNIEXPORT void JNICALL Java_io_tsduck_TSProcessor_abort(JNIEnv*, jobject);

    // Method: io.tsduck.TSProcessor.waitForTermination
    // Signature: ()V
    JNIEXPORT void JNICALL Java_io_tsduck_TSProcessor_waitForTermination(JNIEnv*, jobject);

    // Method: io.tsduck.TSProcessor.delete
    // Signature: ()V
    JNIEXPORT void JNICALL Java_io_tsduck_TSProcessor_delete(JNIEnv*, jobject);
}

//----------------------------------------------------------------------------
// Implementation of native methods.
//----------------------------------------------------------------------------

JNIEXPORT void JNICALL Java_io_tsduck_TSProcessor_initNativeObject(JNIEnv* env, jobject obj, jobject jreport)
{
    // Make sure we do not allocate twice (and lose previous instance).
    ts::TSProcessor* tsp = ts::jni::GetPointerField<ts::TSProcessor>(env, obj, "nativeObject");
    if (tsp == nullptr) {
        ts::Report* report = ts::jni::GetPointerField<ts::Report>(env, jreport, "nativeObject");
        if (report == nullptr) {
            report = ts::NullReport::Instance();
        }
        ts::jni::SetPointerField(env, obj, "nativeObject", new ts::TSProcessor(*report));
    }
}

JNIEXPORT void JNICALL Java_io_tsduck_TSProcessor_abort(JNIEnv* env, jobject obj)
{
    ts::TSProcessor* tsp = ts::jni::GetPointerField<ts::TSProcessor>(env, obj, "nativeObject");
    if (tsp != nullptr) {
        tsp->abort();
    }
}

JNIEXPORT void JNICALL Java_io_tsduck_TSProcessor_waitForTermination(JNIEnv* env, jobject obj)
{
    ts::TSProcessor* tsp = ts::jni::GetPointerField<ts::TSProcessor>(env, obj, "nativeObject");
    if (tsp != nullptr) {
        tsp->waitForTermination();
    }
}

JNIEXPORT void JNICALL Java_io_tsduck_TSProcessor_delete(JNIEnv* env, jobject obj)
{
    ts::TSProcessor* tsp = ts::jni::GetPointerField<ts::TSProcessor>(env, obj, "nativeObject");
    if (tsp != nullptr) {
        delete tsp;
        ts::jni::SetLongField(env, obj, "nativeObject", 0);
    }
}

//----------------------------------------------------------------------------
// Get a plugin description from a Java array of string.
//----------------------------------------------------------------------------

static bool GetPluginOption(JNIEnv* env, jobjectArray strings, ts::PluginOptions& plugin)
{
    plugin.clear();
    if (env == nullptr || strings == nullptr || env->ExceptionCheck()) {
        return false;
    }
    const jsize count = env->GetArrayLength(strings);
    if (count > 0) {
        plugin.name = ts::jni::ToUString(env, jstring(env->GetObjectArrayElement(strings, 0)));
        plugin.args.resize(size_t(count - 1));
        for (jsize i = 1; i < count; ++i) {
            plugin.args[i-1] = ts::jni::ToUString(env, jstring(env->GetObjectArrayElement(strings, i)));
        }
    }
    return !plugin.name.empty();
}


//----------------------------------------------------------------------------
// Start method: the parameters are fetched from the Java object fields.
//----------------------------------------------------------------------------

JNIEXPORT jboolean JNICALL Java_io_tsduck_TSProcessor_start(JNIEnv* env, jobject obj)
{
    ts::TSProcessor* tsp = ts::jni::GetPointerField<ts::TSProcessor>(env, obj, "nativeObject");
    if (tsp == nullptr) {
        return false;
    }

    // Build TSProcessor arguments.
    ts::TSProcessorArgs args;
    args.monitor = ts::jni::GetBoolField(env, obj, "monitor");
    args.ignore_jt = ts::jni::GetBoolField(env, obj, "ignoreJointTermination");
    args.log_plugin_index = ts::jni::GetBoolField(env, obj, "logPluginIndex");
    args.ts_buffer_size = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "bufferSize")));
    if (args.ts_buffer_size == 0) {
        args.ts_buffer_size = ts::TSProcessorArgs::DEFAULT_BUFFER_SIZE;
    }
    args.max_flush_pkt = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "maxFlushedPackets")));
    args.max_input_pkt = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "maxInputPackets")));
    args.init_input_pkt = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "initialInputPackets")));
    args.instuff_nullpkt = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "addInputStuffingNull")));
    args.instuff_inpkt = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "addInputStuffingInput")));
    args.instuff_start = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "addStartStuffing")));
    args.instuff_stop = size_t(std::max<jint>(0, ts::jni::GetIntField(env, obj, "addStopStuffing")));
    args.fixed_bitrate = ts::BitRate(std::max<jint>(0, ts::jni::GetIntField(env, obj, "bitrate")));
    args.bitrate_adj = ts::MilliSecond(std::max<jint>(0, ts::jni::GetIntField(env, obj, "bitrateAdjustInterval")));
    args.receive_timeout = ts::MilliSecond(std::max<jint>(0, ts::jni::GetIntField(env, obj, "receiveTimeout")));
    args.app_name = ts::jni::GetStringField(env, obj, "appName");

    // Get plugins description.
    // Note: The packet processor plugin can be null (no plugin)
    // but the presence of the input and output plugin is required.
    const jobjectArray jplugins = jobjectArray(ts::jni::GetObjectField(env, obj, "plugins", JCS_ARRAY(JCS_ARRAY(JCS_STRING))));
    bool ok = GetPluginOption(env, jobjectArray(ts::jni::GetObjectField(env, obj, "input", JCS_ARRAY(JCS_STRING))), args.input) &&
              GetPluginOption(env, jobjectArray(ts::jni::GetObjectField(env, obj, "output", JCS_ARRAY(JCS_STRING))), args.output);
    const jsize count = jplugins != nullptr ? env->GetArrayLength(jplugins) : 0;
    args.plugins.resize(size_t(count));
    for (jsize i = 0; ok && i < count; ++i) {
        ok = GetPluginOption(env, jobjectArray(env->GetObjectArrayElement(jplugins, i)), args.plugins[i]);
    }

    // Debug message.
    if (tsp->report().debug()) {
        ts::UString cmd(args.app_name);
        cmd.append(u" ");
        cmd.append(args.input.toString(ts::PluginType::INPUT));
        for (auto it = args.plugins.begin(); it != args.plugins.end(); ++it) {
            cmd.append(u" ");
            cmd.append(it->toString(ts::PluginType::PROCESSOR));
        }
        cmd.append(u" ");
        cmd.append(args.output.toString(ts::PluginType::OUTPUT));
        tsp->report().debug(u"starting: %s", {cmd});
    }

    // Finally start the TSProcessor.
    return ok = ok && tsp->start(args);
}

#endif // TS_NO_JAVA

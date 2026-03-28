/*
    Atto Engine - Audio System Implementation
    OpenAL-based audio for sound effects and music
*/

#include "atto_audio.h"
#include "atto_log.h"

#include "atto_engine.h"

#include <AL/al.h>
#include <AL/alc.h>

#include <cstring>
#include <cstdlib>
#include <format>

namespace atto {

    void SoundCollection::Initialize() {
        Initialize( &Engine::Get().GetAudioSystem(), &Engine::Get().GetRNG() );
    }

    void SoundCollection::Initialize( AudioSystem * audioSystem, RNG * rng ) {
        this->audioSystem = audioSystem;
        this->rng = rng;
    }

    void SoundCollection::LoadSounds( const std::vector<const char *> & names, bool mono ) {
        ATTO_ASSERT( audioSystem != nullptr, "Audio system is null" );
        ATTO_ASSERT( rng != nullptr, "RNG is null" );

        for ( const char * name : names ) {
            buffers.push_back( audioSystem->GetOrLoadSound( std::format( "{}", name ).c_str(), mono ) );
            this->names.push_back( std::string( name ) );
        }
    }

    AudioSourceHandle SoundCollection::Play( f32 volume, bool loop ) {
        ATTO_ASSERT( audioSystem != nullptr, "Audio system is null" );
        ATTO_ASSERT( rng != nullptr, "RNG is null" );

        const i32 idx = rng->Signed32( 0, static_cast< i32 >( buffers.size() ) - 1 );
        return audioSystem->PlaySound( buffers[idx], volume, loop );
    }

    AudioSourceHandle SoundCollection::PlayAt( Vec3 position, f32 volume, bool loop ) {
        ATTO_ASSERT( audioSystem != nullptr, "Audio system is null" );
        ATTO_ASSERT( rng != nullptr, "RNG is null" );

        const i32 idx = rng->Signed32( 0, static_cast< i32 >( buffers.size() ) - 1 );
        return audioSystem->PlaySoundAt( buffers[idx], position, volume, loop );
    }

    // Helper to check OpenAL errors
    static bool CheckALError( const char * operation ) {
        ALenum error = alGetError();
        if ( error != AL_NO_ERROR ) {
            const char * errorStr = "Unknown error";
            switch ( error ) {
            case AL_INVALID_NAME: errorStr = "AL_INVALID_NAME"; break;
            case AL_INVALID_ENUM: errorStr = "AL_INVALID_ENUM"; break;
            case AL_INVALID_VALUE: errorStr = "AL_INVALID_VALUE"; break;
            case AL_INVALID_OPERATION: errorStr = "AL_INVALID_OPERATION"; break;
            case AL_OUT_OF_MEMORY: errorStr = "AL_OUT_OF_MEMORY"; break;
            }
            LOG_ERROR( "OpenAL error during %s: %s", operation, errorStr );
            return false;
        }
        return true;
    }

    //static bool CheckALCError( ALCdevice * device, const char * operation ) {
    //    ALCenum error = alcGetError( device );
    //    if ( error != ALC_NO_ERROR ) {
    //        const char * errorStr = "Unknown error";
    //        switch ( error ) {
    //        case ALC_INVALID_DEVICE: errorStr = "ALC_INVALID_DEVICE"; break;
    //        case ALC_INVALID_CONTEXT: errorStr = "ALC_INVALID_CONTEXT"; break;
    //        case ALC_INVALID_ENUM: errorStr = "ALC_INVALID_ENUM"; break;
    //        case ALC_INVALID_VALUE: errorStr = "ALC_INVALID_VALUE"; break;
    //        case ALC_OUT_OF_MEMORY: errorStr = "ALC_OUT_OF_MEMORY"; break;
    //        }
    //        LOG_ERROR( "OpenAL context error during %s: %s", operation, errorStr );
    //        return false;
    //    }
    //    return true;
    //}

    bool AudioSystem::Initialize( const AudioConfig & cfg ) {
        if ( initialized ) {
            LOG_WARN( "AudioSystem already initialized" );
            return false;
        }

        config = cfg;

        // Open default audio device
        ALCdevice * alDevice = alcOpenDevice( nullptr );
        if ( !alDevice ) {
            LOG_ERROR( "Failed to open OpenAL audio device" );
            return false;
        }
        device = alDevice;

        // Create audio context
        ALCcontext * alContext = alcCreateContext( alDevice, nullptr );
        if ( !alContext ) {
            LOG_ERROR( "Failed to create OpenAL context" );
            alcCloseDevice( alDevice );
            device = nullptr;
            return false;
        }
        context = alContext;

        // Make context current
        if ( !alcMakeContextCurrent( alContext ) ) {
            LOG_ERROR( "Failed to make OpenAL context current" );
            alcDestroyContext( alContext );
            alcCloseDevice( alDevice );
            device = nullptr;
            context = nullptr;
            return false;
        }

        // Clear any existing errors
        alGetError();

        // Set distance model for 3D audio
        alDistanceModel( AL_INVERSE_DISTANCE_CLAMPED );
        CheckALError( "setting distance model" );

        // Configure doppler effect
        alDopplerFactor( config.dopplerFactor );
        alSpeedOfSound( config.speedOfSound );

        // Initialize listener at origin for 2D mode by default
        SetListener2D();

        // Pre-create OpenAL sources
        for ( u32 i = 0; i < MAX_SOURCES; ++i ) {
            ALuint sourceId = 0;
            alGenSources( 1, &sourceId );
            if ( !CheckALError( "generating source" ) ) {
                LOG_WARN( "Could only create %u audio sources", i );
                break;
            }
            sources[i].alSource = sourceId;
            sources[i].volume = 1.0f;
            sources[i].pitch = 1.0f;
            sources[i].looping = false;
            sources[i].positional = false;
            sources[i].position = Vec3( 0.0f );
            sourceGenerations[i] = 0;  // Not in use yet
        }

        initialized = true;

        // Log device info
        const ALCchar * deviceName = alcGetString( alDevice, ALC_DEVICE_SPECIFIER );
        LOG_INFO( "OpenAL initialized: %s", deviceName ? deviceName : "Unknown device" );
        LOG_INFO( "OpenAL Version: %s", alGetString( AL_VERSION ) );
        LOG_INFO( "OpenAL Renderer: %s", alGetString( AL_RENDERER ) );

        return true;
    }

    void AudioSystem::Shutdown() {
        if ( !initialized ) return;

        // Stop all sounds
        StopAll();

        // Delete all sources
        for ( u32 i = 0; i < MAX_SOURCES; ++i ) {
            if ( sources[i].alSource != 0 ) {
                alDeleteSources( 1, &sources[i].alSource );
                sources[i].alSource = 0;
            }
            sourceGenerations[i] = 0;
        }

        // Delete all buffers
        for ( u32 i = 0; i < MAX_BUFFERS; ++i ) {
            if ( buffers[i].alBuffer != 0 ) {
                alDeleteBuffers( 1, &buffers[i].alBuffer );
                buffers[i].alBuffer = 0;
            }
        }

        // Destroy context and close device
        ALCcontext * alContext = static_cast<ALCcontext *>( context );
        ALCdevice * alDevice = static_cast<ALCdevice *>( device );

        alcMakeContextCurrent( nullptr );

        if ( alContext ) {
            alcDestroyContext( alContext );
        }

        if ( alDevice ) {
            alcCloseDevice( alDevice );
        }

        device = nullptr;
        context = nullptr;
        initialized = false;

        LOG_INFO( "AudioSystem shutdown" );
    }

    bool AudioSystem::CreateBuffer( AudioBuffer * handle, const i16 * data, i32 sampleCount, i32 channels, i32 sampleRate ) {
        AudioBuffer & buffer = *handle;

        // Determine OpenAL format
        ALenum format = AL_FORMAT_MONO16;
        if ( channels == 2 ) {
            format = AL_FORMAT_STEREO16;
        }
        else if ( channels != 1 ) {
            LOG_ERROR( "Unsupported channel count: %d (only mono and stereo supported)", channels );
            return false;
        }

        // Generate OpenAL buffer
        ALuint alBuffer = 0;
        alGenBuffers( 1, &alBuffer );
        if ( !CheckALError( "generating buffer" ) ) {
            return false;
        }

        // Upload data to OpenAL
        ALsizei dataSize = sampleCount * sizeof( i16 );
        alBufferData( alBuffer, format, data, dataSize, sampleRate );
        if ( !CheckALError( "uploading buffer data" ) ) {
            alDeleteBuffers( 1, &alBuffer );
            return false;
        }

        buffer.alBuffer = alBuffer;
        buffer.channels = channels;
        buffer.sampleRate = sampleRate;
        buffer.bitsPerSample = 16;
        buffer.duration = static_cast<f32>(sampleCount / channels) / static_cast<f32>(sampleRate);

        return true;
    }

    void AudioSystem::DestroyBuffer( AudioBuffer * buffer ) {
        for ( u32 i = 0; i < MAX_SOURCES; ++i ) {
            if ( sources[i].buffer == buffer ) {
                alSourceStop( sources[i].alSource );
                alSourcei( sources[i].alSource, AL_BUFFER, 0 );
                sources[i].buffer = nullptr;
            }
        }

        if ( buffer->alBuffer != 0 ) {
            alDeleteBuffers( 1, &buffer->alBuffer );
            buffer->alBuffer = 0;
        }

        buffer->channels = 0;
        buffer->sampleRate = 0;
        buffer->bitsPerSample = 0;
        buffer->duration = 0.0f;
    }

    AudioBuffer * AudioSystem::GetOrLoadSound( const char * path, bool mono ) {
        AudioBuffer * buffer = GetOrLoadSoundHandle( path, mono );
        return buffer;
    }

    AudioBuffer * AudioSystem::GetLoadedSound( const char * path ) const {
        // Search for existing buffer with this name
        for ( u32 i = 1; i < MAX_BUFFERS; ++i ) {
            if ( buffers[i].alBuffer != 0 && buffers[i].name == path ) {
                return const_cast<AudioBuffer *>( &buffers[i] );
            }
        }

        return nullptr;
    }

    AudioBuffer * AudioSystem::GetOrLoadSoundHandle( const char * path, bool mono ) {
        // Cache key includes ":mono" suffix when downmixed
        std::string cacheKey = mono ? std::string( path ) + ":mono" : std::string( path );

        // Search for existing buffer with this name
        for ( u32 i = 1; i < MAX_BUFFERS; ++i ) {
            if ( buffers[i].alBuffer != 0 && buffers[i].name == cacheKey ) {
                return &buffers[i];
            }
        }

        if ( !initialized ) {
            LOG_ERROR( "AudioSystem not initialized" );
            return nullptr;
        }

        // Load raw audio data into serializer
        BinarySerializer serializer( true );
        if ( !Engine::Get().GetAssetManager().LoadSound( path, mono, serializer ) ) {
            return nullptr;
        }
        serializer.Reset( false );

        // Read back from serializer
        std::string name;
        i32 channels = 0;
        i32 sampleRate = 0;
        i32 sampleCount = 0;
        serializer( "Name", name );
        serializer( "Channels", channels );
        serializer( "SampleRate", sampleRate );
        serializer( "SampleCount", sampleCount );

        std::vector<u8> sampleBytes;
        serializer( "Samples", sampleBytes );

        // Find a free buffer slot
        u32 bufferIndex = 0;
        bool found = false;
        for ( u32 i = 1; i < MAX_BUFFERS; ++i ) {
            u32 idx = ( nextBufferIndex + i ) % MAX_BUFFERS;
            if ( idx == 0 ) idx = 1;
            if ( buffers[idx].alBuffer == 0 ) {
                bufferIndex = idx;
                found = true;
                break;
            }
        }

        if ( !found ) {
            LOG_ERROR( "No free audio buffer slots available" );
            return nullptr;
        }

        AudioBuffer * handle = &buffers[bufferIndex];
        handle->name = name;
        bool success = CreateBuffer( handle, reinterpret_cast<const i16 *>( sampleBytes.data() ), sampleCount, channels, sampleRate );

        if ( !success ) {
            return nullptr;
        }

        nextBufferIndex = bufferIndex;

        LOG_DEBUG( "Loaded sound: %s (%.2fs, %dHz, %dch)", path, handle->duration, sampleRate, channels );

        return handle;
    }

    AudioSourceHandle AudioSystem::CreateSource() {
        // Find a free source (one not currently in use)
        for ( u32 i = 0; i < MAX_SOURCES; ++i ) {
            u32 idx = (nextSourceIndex + i) % MAX_SOURCES;
            if ( sourceGenerations[idx] == 0 && sources[idx].alSource != 0 ) {
                sourceGenerations[idx] = 1;  // Mark as in use
                nextSourceIndex = (idx + 1) % MAX_SOURCES;

                AudioSourceHandle handle;
                handle.index = idx;
                handle.generation = sourceGenerations[idx];

                // Reset source to default state
                AudioSource & src = sources[idx];
                alSourceStop( src.alSource );
                alSourcei( src.alSource, AL_BUFFER, 0 );
                alSourcef( src.alSource, AL_GAIN, 1.0f );
                alSourcef( src.alSource, AL_PITCH, 1.0f );
                alSourcei( src.alSource, AL_LOOPING, AL_FALSE );
                alSourcei( src.alSource, AL_SOURCE_RELATIVE, AL_TRUE );
                alSource3f( src.alSource, AL_POSITION, 0.0f, 0.0f, 0.0f );
                alSource3f( src.alSource, AL_VELOCITY, 0.0f, 0.0f, 0.0f );

                src.buffer = nullptr;
                src.volume = 1.0f;
                src.pitch = 1.0f;
                src.looping = false;
                src.positional = false;
                src.position = Vec3( 0.0f );

                return handle;
            }
        }

        LOG_WARN( "No free audio sources available" );
        return AudioSourceHandle{};
    }

    void AudioSystem::DestroySource( AudioSourceHandle handle ) {
        AudioSource * source = GetSource( handle );
        if ( !source ) return;

        alSourceStop( source->alSource );
        alSourcei( source->alSource, AL_BUFFER, 0 );
        CheckALError( "destroying source" );

        source->buffer = nullptr;
        sourceGenerations[handle.index] = 0;  // Mark as free
    }

    AudioSource * AudioSystem::GetSource( AudioSourceHandle handle ) {
        if ( !handle.IsValid() || handle.index >= MAX_SOURCES ) {
            return nullptr;
        }
        if ( sourceGenerations[handle.index] != handle.generation ) {
            return nullptr;
        }
        return &sources[handle.index];
    }

    AudioSourceHandle AudioSystem::FindFreeSource() {
        return CreateSource();
    }

    void AudioSystem::Play( AudioSourceHandle handle ) {
        if ( mute ) {
            return;
        }

        AudioSource * source = GetSource( handle );
        if ( !source ) return;

        alSourcePlay( source->alSource );
        CheckALError( "playing source" );
    }

    void AudioSystem::Stop( AudioSourceHandle handle ) {
        AudioSource * source = GetSource( handle );
        if ( !source ) return;

        alSourceStop( source->alSource );
        CheckALError( "stopping source" );
    }

    void AudioSystem::Pause( AudioSourceHandle handle ) {
        AudioSource * source = GetSource( handle );
        if ( !source ) return;

        alSourcePause( source->alSource );
        CheckALError( "pausing source" );
    }

    void AudioSystem::Resume( AudioSourceHandle handle ) {
        AudioSource * source = GetSource( handle );
        if ( !source ) return;

        ALint state;
        alGetSourcei( source->alSource, AL_SOURCE_STATE, &state );
        if ( state == AL_PAUSED ) {
            alSourcePlay( source->alSource );
            CheckALError( "resuming source" );
        }
    }

    AudioState AudioSystem::GetState( AudioSourceHandle handle ) {
        AudioSource * source = GetSource( handle );
        if ( !source ) return AudioState::Stopped;

        ALint state;
        alGetSourcei( source->alSource, AL_SOURCE_STATE, &state );

        switch ( state ) {
        case AL_PLAYING: return AudioState::Playing;
        case AL_PAUSED: return AudioState::Paused;
        default: return AudioState::Stopped;
        }
    }

    void AudioSystem::SetBuffer( AudioSourceHandle handle, AudioBuffer * buffer ) {
        AudioSource * source = GetSource( handle );
        if ( !source ) return;

        // Stop current playback
        alSourceStop( source->alSource );

        if ( buffer ) {
            alSourcei( source->alSource, AL_BUFFER, buffer->alBuffer );
            source->buffer = buffer;
        }
        else {
            alSourcei( source->alSource, AL_BUFFER, 0 );
            source->buffer = nullptr;
        }

        CheckALError( "setting buffer" );
    }

    void AudioSystem::SetVolume( AudioSourceHandle handle, f32 volume ) {
        AudioSource * source = GetSource( handle );
        if ( !source ) return;

        source->volume = volume < 0.0f ? 0.0f : volume;
        UpdateSourceVolume( handle );
    }

    void AudioSystem::UpdateSourceVolume( AudioSourceHandle handle ) {
        AudioSource * source = GetSource( handle );
        if ( !source ) return;

        f32 finalVolume = source->volume * config.masterVolume * config.sfxVolume;
        alSourcef( source->alSource, AL_GAIN, finalVolume );
    }

    void AudioSystem::SetPitch( AudioSourceHandle handle, f32 pitch ) {
        AudioSource * source = GetSource( handle );
        if ( !source ) return;

        source->pitch = pitch < 0.01f ? 0.01f : pitch;
        alSourcef( source->alSource, AL_PITCH, source->pitch );
        CheckALError( "setting pitch" );
    }

    void AudioSystem::SetLooping( AudioSourceHandle handle, bool loop ) {
        AudioSource * source = GetSource( handle );
        if ( !source ) return;

        source->looping = loop;
        alSourcei( source->alSource, AL_LOOPING, loop ? AL_TRUE : AL_FALSE );
        CheckALError( "setting looping" );
    }

    void AudioSystem::SetPosition( AudioSourceHandle handle, const Vec3 & position ) {
        AudioSource * source = GetSource( handle );
        if ( !source ) return;

        source->position = position;
        if ( source->positional ) {
            alSource3f( source->alSource, AL_POSITION, position.x, position.y, position.z );
            CheckALError( "setting position" );
        }
    }

    void AudioSystem::SetVelocity( AudioSourceHandle handle, const Vec3 & velocity ) {
        AudioSource * source = GetSource( handle );
        if ( !source ) return;

        alSource3f( source->alSource, AL_VELOCITY, velocity.x, velocity.y, velocity.z );
        CheckALError( "setting velocity" );
    }

    void AudioSystem::SetPositional( AudioSourceHandle handle, bool positional ) {
        AudioSource * source = GetSource( handle );
        if ( !source ) return;

        source->positional = positional;

        if ( positional ) {
            // Use absolute world position
            alSourcei( source->alSource, AL_SOURCE_RELATIVE, AL_FALSE );
            alSource3f( source->alSource, AL_POSITION, source->position.x, source->position.y, source->position.z );
        }
        else {
            // Relative to listener (effectively 2D)
            alSourcei( source->alSource, AL_SOURCE_RELATIVE, AL_TRUE );
            alSource3f( source->alSource, AL_POSITION, 0.0f, 0.0f, 0.0f );
        }
        CheckALError( "setting positional" );
    }

    AudioSourceHandle AudioSystem::PlaySound( AudioBuffer * buffer, f32 volume, bool loop ) {
        AudioSourceHandle source = CreateSource();
        if ( !source.IsValid() ) {
            return AudioSourceHandle{};
        }

        SetBuffer( source, buffer );
        SetVolume( source, volume );
        SetLooping( source, loop );
        Play( source );

        // Track non-looping sounds for automatic cleanup
        if ( !loop && oneshotCount < MAX_ONESHOT ) {
            oneshotSources[oneshotCount++] = source;
        }

        return source;
    }

    AudioSourceHandle AudioSystem::PlaySoundAt( AudioBuffer * buffer, const Vec3 & position, f32 volume, bool loop ) {
        AudioSourceHandle source = CreateSource();
        if ( !source.IsValid() ) {
            return AudioSourceHandle{};
        }

        SetBuffer( source, buffer );
        SetVolume( source, volume );
        SetLooping( source, loop );
        SetPosition( source, position );
        SetPositional( source, true );
        Play( source );

        // Track non-looping sounds for automatic cleanup
        if ( !loop && oneshotCount < MAX_ONESHOT ) {
            oneshotSources[oneshotCount++] = source;
        }

        return source;
    }

    void AudioSystem::SetListenerPosition( const Vec3 & position ) {
        alListener3f( AL_POSITION, position.x, position.y, position.z );
        CheckALError( "setting listener position" );
    }

    void AudioSystem::SetListenerVelocity( const Vec3 & velocity ) {
        alListener3f( AL_VELOCITY, velocity.x, velocity.y, velocity.z );
        CheckALError( "setting listener velocity" );
    }

    void AudioSystem::SetListenerOrientation( const Vec3 & forward, const Vec3 & up ) {
        ALfloat orientation[6] = {
            forward.x, forward.y, forward.z,
            up.x, up.y, up.z
        };
        alListenerfv( AL_ORIENTATION, orientation );
        CheckALError( "setting listener orientation" );
    }

    void AudioSystem::SetListener2D() {
        SetListenerPosition( Vec3( 0.0f, 0.0f, 0.0f ) );
        SetListenerVelocity( Vec3( 0.0f, 0.0f, 0.0f ) );
        // Looking down -Z axis, up is +Y (standard 2D setup)
        SetListenerOrientation( Vec3( 0.0f, 0.0f, -1.0f ), Vec3( 0.0f, 1.0f, 0.0f ) );
    }

    void AudioSystem::SetMasterVolume( f32 volume ) {
        config.masterVolume = volume < 0.0f ? 0.0f : (volume > 1.0f ? 1.0f : volume);
        // Update all source volumes
        for ( u32 i = 0; i < MAX_SOURCES; ++i ) {
            if ( sourceGenerations[i] != 0 ) {
                AudioSourceHandle handle;
                handle.index = i;
                handle.generation = sourceGenerations[i];
                UpdateSourceVolume( handle );
            }
        }
    }

    void AudioSystem::SetSFXVolume( f32 volume ) {
        config.sfxVolume = volume < 0.0f ? 0.0f : (volume > 1.0f ? 1.0f : volume);
        // Update all source volumes
        for ( u32 i = 0; i < MAX_SOURCES; ++i ) {
            if ( sourceGenerations[i] != 0 ) {
                AudioSourceHandle handle;
                handle.index = i;
                handle.generation = sourceGenerations[i];
                UpdateSourceVolume( handle );
            }
        }
    }

    void AudioSystem::SetMusicVolume( f32 volume ) {
        config.musicVolume = volume < 0.0f ? 0.0f : (volume > 1.0f ? 1.0f : volume);
    }

    void AudioSystem::StopAll() {
        for ( u32 i = 0; i < MAX_SOURCES; ++i ) {
            if ( sources[i].alSource != 0 ) {
                alSourceStop( sources[i].alSource );
            }
        }
        oneshotCount = 0;
    }

    void AudioSystem::Update() {
        if ( !initialized ) return;

        // Clean up finished one-shot sounds
        u32 writeIndex = 0;
        for ( u32 i = 0; i < oneshotCount; ++i ) {
            AudioSourceHandle handle = oneshotSources[i];
            AudioState state = GetState( handle );

            if ( state == AudioState::Stopped ) {
                // Sound finished, free the source
                DestroySource( handle );
            }
            else {
                // Still playing, keep tracking
                oneshotSources[writeIndex++] = handle;
            }
        }
        oneshotCount = writeIndex;
    }

} // namespace atto

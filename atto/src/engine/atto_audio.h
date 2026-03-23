#pragma once

/*
    Atto Engine - Audio System
    OpenAL-based audio layer for sound effects and music
    Compatible with Emscripten for browser builds
*/

#include "atto_core.h"
#include "atto_math.h"

namespace atto {

    // Forward declarations
    struct AudioBuffer;
    struct AudioSource;

    class RNG;
    class AudioSystem;

    // Handle types for audio resources
    using AudioSourceHandle = Handle<AudioSource>;

    // Audio buffer - stores loaded sound data
    struct AudioBuffer {
        u32 alBuffer = 0;           // OpenAL buffer ID
        i32 channels = 0;
        i32 sampleRate = 0;
        i32 bitsPerSample = 0;
        f32 duration = 0.0f;        // Duration in seconds

        std::string name;

        bool IsValid() const { return alBuffer != 0; }
    };

    class SoundCollection {
    public:
        void Initialize();
        void Initialize( AudioSystem * audioSystem, RNG * rng );
        void LoadSounds( const std::vector<const char *> & names );
        void Play( f32 volume = 1.0f );
        void PlayAt( Vec3 position, f32 volume = 1.0f );

    private:
        RNG *                       rng = nullptr;
        AudioSystem *               audioSystem = nullptr;
        std::vector<AudioBuffer *>  buffers;
        std::vector<std::string>    names;
    };

    // Audio source state
    enum class AudioState : u8 {
        Stopped = 0,
        Playing,
        Paused
    };

    // Audio source - an instance playing a sound
    struct AudioSource {
        u32 alSource = 0;           // OpenAL source ID
        AudioBuffer * buffer = nullptr;    // Currently attached buffer
        bool looping = false;
        f32 volume = 1.0f;
        f32 pitch = 1.0f;
        Vec3 position = Vec3( 0.0f );
        bool positional = false;    // If true, uses 3D positioning

        bool IsValid() const { return alSource != 0; }
    };

    // Audio configuration
    struct AudioConfig {
        f32 masterVolume = 1.0f;
        f32 sfxVolume = 1.0f;
        f32 musicVolume = 1.0f;
        f32 dopplerFactor = 1.0f;
        f32 speedOfSound = 343.3f;  // meters per second
    };

    // Audio system - manages all audio resources and playback
    class AudioSystem {
    public:
        bool                Initialize( const AudioConfig & config = AudioConfig() );
        void                Shutdown();

        bool                IsInitialized() const { return initialized; }
        bool                IsMuted() const { return mute; }
        void                SetMuted( bool muted ) { mute = muted; }
        void                ToggleMuted() { mute = !mute; }

        // Buffer management - load sound data
        AudioBuffer *       LoadWAV( const char * path );
        AudioBuffer *       LoadOGG( const char * path );
        AudioBuffer *       LoadSound( const char * path );  // Auto-detects format
        void                DestroyBuffer( AudioBuffer * buffer  );

        AudioBuffer *       GetOrLoadSound( const char * path );
        AudioBuffer *       GetLoadedSound( const char * path ) const;
        AudioBuffer *       GetOrLoadSoundHandle( const char * path );

        // Source management - create playback instances
        AudioSourceHandle   CreateSource();
        void                DestroySource( AudioSourceHandle handle );
        AudioSource *       GetSource( AudioSourceHandle handle );

        // Playback control
        void                Play( AudioSourceHandle source );
        void                Stop( AudioSourceHandle source );
        void                Pause( AudioSourceHandle source );
        void                Resume( AudioSourceHandle source );
        AudioState          GetState( AudioSourceHandle source );

        // Source properties
        void                SetBuffer( AudioSourceHandle source, AudioBuffer * buffer );
        void                SetVolume( AudioSourceHandle source, f32 volume );
        void                SetPitch( AudioSourceHandle source, f32 pitch );
        void                SetLooping( AudioSourceHandle source, bool loop );
        void                SetPosition( AudioSourceHandle source, const Vec3 & position );
        void                SetVelocity( AudioSourceHandle source, const Vec3 & velocity );
        void                SetPositional( AudioSourceHandle source, bool positional );

        // Convenience - play a sound immediately
        // Returns source handle for further control, or invalid handle on failure
        AudioSourceHandle   PlaySound( AudioBuffer * buffer, f32 volume = 1.0f, bool loop = false );
        AudioSourceHandle   PlaySoundAt( AudioBuffer * buffer, const Vec3 & position, f32 volume = 1.0f, bool loop = false );

        // Listener (usually follows camera)
        void                SetListenerPosition( const Vec3 & position );
        void                SetListenerVelocity( const Vec3 & velocity );
        void                SetListenerOrientation( const Vec3 & forward, const Vec3 & up );

        // For 2D games - sets listener at origin looking down -Z
        void                SetListener2D();

        // Volume control
        void                SetMasterVolume( f32 volume );
        void                SetSFXVolume( f32 volume );
        void                SetMusicVolume( f32 volume );
        f32                 GetMasterVolume() const { return config.masterVolume; }
        f32                 GetSFXVolume() const { return config.sfxVolume; }
        f32                 GetMusicVolume() const { return config.musicVolume; }

        // Stop all sounds
        void                StopAll();

        // Update - call each frame to clean up finished one-shot sounds
        void                Update();

    private:

        bool CreateBuffer( AudioBuffer * handle, const i16 * data, i32 sampleCount, i32 channels, i32 sampleRate );
        AudioSourceHandle FindFreeSource();
        void UpdateSourceVolume( AudioSourceHandle handle );

        bool initialized = false;
        void * device = nullptr;    // ALCdevice*
        void * context = nullptr;   // ALCcontext*

        bool mute = false;

        AudioConfig config;

        // Buffer storage
        static constexpr u32 MAX_BUFFERS = 256;
        u32 nextBufferIndex = 1;
        AudioBuffer buffers[MAX_BUFFERS];

        // Source storage
        static constexpr u32 MAX_SOURCES = 64;
        AudioSource sources[MAX_SOURCES];
        u32 sourceGenerations[MAX_SOURCES] = {};
        u32 nextSourceIndex = 0;

        // Track one-shot sounds for automatic cleanup
        static constexpr u32 MAX_ONESHOT = 32;
        AudioSourceHandle oneshotSources[MAX_ONESHOT];
        u32 oneshotCount = 0;
    };

} // namespace atto

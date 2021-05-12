using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
using UnityEngine;

namespace MusicSpatializerPitchCorrection
{
    

    public class PitchShifter
    {
        public IntPtr nativeState = IntPtr.Zero;

        const string stftRecoverLibLocation = "Libs/Native/stftRecoverLib.dll";

        [DllImport(stftRecoverLibLocation, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr InPlacePitchShiftInit(int channels);

        [DllImport(stftRecoverLibLocation, CallingConvention = CallingConvention.Cdecl)]
        public static extern void InPlacePitchShiftFree(IntPtr state);

        [DllImport(stftRecoverLibLocation, CallingConvention = CallingConvention.Cdecl)]
        public static extern void InPlacePitchShiftRun(IntPtr state, float[] data, int channelLength, float detuneMultiplier);
        
        [DllImport(stftRecoverLibLocation, CallingConvention = CallingConvention.Cdecl)]
        public static extern void InPlacePitchShiftRunUnity(IntPtr state, float[] data, int channelLength, float detuneMultiplier);

        [DllImport(stftRecoverLibLocation, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Noise(float[] data, int length);

        [DllImport(stftRecoverLibLocation, CallingConvention = CallingConvention.Cdecl)]
        public static extern void DestroyerOfEars(float[] data, int length);

        [DllImport(stftRecoverLibLocation, CallingConvention = CallingConvention.Cdecl)]
        public static extern void Scilence(float[] data, int length);


        public void Shift(float[] data,int channels, float detuneMultiplier)
        {
            //Console.WriteLine("before init: {0}", data[100]);
            if (nativeState == IntPtr.Zero)
            {
                nativeState = InPlacePitchShiftInit(channels);
            }

            //Console.WriteLine("number from data before : {0}", data[100]);
            InPlacePitchShiftRun(nativeState, data, data.Length / channels, detuneMultiplier);
            for(int i=0;i< data.Length; i++) { if (Single.IsNaN(data[i])) { Console.WriteLine("====ERROR==== null at {0} in pitched data", i);break; } }
            //Console.WriteLine("number from data after : {0}", data[100]);
            //DestroyerOfEars(data, data.Length);
        }

        ~PitchShifter() 
        {
            if (nativeState != IntPtr.Zero)
            {
                InPlacePitchShiftFree(nativeState);
                nativeState = IntPtr.Zero;
                //Console.WriteLine("PitchShifter freed");
            }
        }
    }

    public class PitchFilter : MonoBehaviour
    {
        public float pitchMultiplier = 1f;
        public AudioSource correctSource;

        public PitchShifter pitchShifter = new PitchShifter();
        //public bool enabled = true;
        // Start is called before the first frame update
        void Start()
        {
            MusicSpatializer.Plugin.LevelFailed += OnFail;
        }

        // Update is called once per frame
        void Update()
        {

        }

        void OnDestroy()
        {
            MusicSpatializer.Plugin.LevelFailed -= OnFail;
        }

        void OnFail()
        {
            correctSource = null;
        }

        void OnAudioFilterRead(float[] data, int channels)
        {
            if (correctSource != null)
            {
                pitchMultiplier = 1 / correctSource.pitch;
            }
            //Console.WriteLine("number from data : {0}", data[100]);
            if(!Mathf.Approximately(pitchMultiplier, 1f)){
                pitchShifter.Shift(data, channels, pitchMultiplier);
            }
            //Console.WriteLine("number from data 2 : {0}", data[100]);
        }
    }
}

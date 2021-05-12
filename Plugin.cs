using BeatSaberMarkupLanguage.Settings;
using IPA;
using IPA.Config;
using IPA.Loader;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using UnityEngine;
using UnityEngine.SceneManagement;
using MusicSpatializerPitchCorrection.Settings;
using MusicSpatializerPitchCorrection.Settings.UI;


namespace MusicSpatializerPitchCorrection
{
    [Plugin(RuntimeOptions.SingleStartInit)]
    public class Plugin {


        public Logger log;
        public const string Name = "Music Spatializer Pitch Correction";

        bool hasRegisteredEvent = false;

        [Init]
        public void Init(Logger logger, Config conf)
        {
            log = logger;
            Configuration.Init(conf);
            SceneManager.sceneLoaded += OnSceneLoaded;
        }
        

        public void OnSceneLoaded(Scene scene, LoadSceneMode sceneMode) {
            if (scene.name == "MenuViewControllers"){ }
        }

        public void LoadSettingsMenu()
        {
            BSMLSettings.instance.AddSettingsMenu("Music Pitch", "MusicSpatializerPitchCorrection.Settings.UI.Views.mainsettings.bsml", MainSettings.instance);
            
        }

        public void AttachPitchFilter(MusicSpatializer.Plugin.PitchHookArgs args)
        {
            if (Configuration.config.enabled)
            {
                PitchFilter pitchFilter = args.songControl.AddComponent<PitchFilter>();
                pitchFilter.correctSource = args.mainSource;
                //Log("PitchFilter Attached");
            }
        }
        
        [OnStart]
        public void OnStart()
        {
            if (hasRegisteredEvent == false)
            {
                MusicSpatializer.Plugin.PitchHook += AttachPitchFilter;
                hasRegisteredEvent = true;
                Log("PitchHook Attached");
                MusicSpatializer.Plugin.SettingUiLoad += LoadSettingsMenu;
            }
        }

        [OnExit]
        public void OnExit()
        {
            
        }

        public static void Log(string format, params object[] args) {
            Console.WriteLine($"[{Name}] " + format, args);
        }



    }


}

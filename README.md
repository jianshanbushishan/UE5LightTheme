# Light Theme Fix

Light Theme Fix is an editor-only Unreal Engine plugin that installs the clean, neutral **Light** theme and repairs low-contrast Slate widgets that a color-theme JSON cannot style reliably.

By default, the plugin also follows Windows' default app mode: Unreal switches to the bundled **Light** theme in Windows light mode and Unreal's standard **Dark** theme in Windows dark mode. Changes are detected while the editor is running.

The current binary release targets **Unreal Engine 5.8 on Win64**. The archive also includes source code so it can be rebuilt against another compatible engine build.

## Preview

The Unreal Editor after enabling the bundled **Light** theme and the plugin's contrast fixes:

![Light theme enabled in Unreal Editor](Resources/Themes/screenshot.png)

## What it fixes

- Focused text in search boxes, numeric fields, editable text boxes, and inline editors
- Combo-box rows and dock-tab foregrounds
- Blueprint graph background, grid, breadcrumbs, node bodies, titles, and execution wires
- Animation Editor timeline rows and labels
- Content Browser hover and selection states
- Tooltip text created after editor startup
- Late editor shutdown without accessing `CoreUObject` after engine exit begins

Every feature can be enabled independently. Colors, luminance threshold, node geometry, timeline colors, Content Browser states, and runtime refresh timing are available under **Editor Preferences > Plugins > Light Theme Fix**. Settings are stored per user and engine version, and are shared by all projects opened with that engine version.

## Installation

Releases are designed to be installed as precompiled external plugins discovered through `UE_ADDITIONAL_PLUGIN_PATHS`. This keeps the plugin outside individual projects and outside the Unreal Engine installation.

1. Download and extract the release archive built for your exact Unreal Engine version.
2. Close all Unreal Editor instances.
3. Run the installer from the extracted plugin directory:

   ```powershell
   .\LightThemeFix\Scripts\InstallExternalPlugin.ps1
   ```

The default installation path is derived from the release descriptor's supported `EngineVersion` (major and minor version):

```text
%LOCALAPPDATA%\UnrealEngine\{SupportedEngineVersion}\ExternalPlugins\LightThemeFix
```

For example, a release whose descriptor declares `EngineVersion` as `5.8.0` uses `5.8` as `{SupportedEngineVersion}`. A future `5.9.0` release will automatically use the `5.9` directory.

The installer preserves existing entries and adds the version-specific `ExternalPlugins` directory to the current user's `UE_ADDITIONAL_PLUGIN_PATHS`. Packaged releases are marked `Installed` and `EnabledByDefault`, so projects do not need their own plugin copy or a `LightThemeFix` entry in each `.uproject` file. Restart Unreal Editor and any launcher process after installation so they inherit the updated environment.

Environment variables are inherited when a process starts. The installer broadcasts the Windows environment-change notification for newly launched desktop processes, but an already-running Epic Games Launcher retains its old environment. Exit it completely—including its tray process—before reopening Unreal Editor. Merely closing and reopening a project from the same launcher process will retain the old plugin search path.

To use another external plugin root:

```powershell
.\LightThemeFix\Scripts\InstallExternalPlugin.ps1 -InstallRoot 'D:\UnrealPlugins\{SupportedEngineVersion}'
```

`UE_ADDITIONAL_PLUGIN_PATHS` is read by every Unreal Editor version launched from that user environment. Each release declares its intended `EngineVersion`, but for strict process-level isolation, install without changing the user environment and set the variable only in the command that launches the matching editor:

```powershell
.\LightThemeFix\Scripts\InstallExternalPlugin.ps1 `
    -InstallRoot 'D:\UnrealPlugins\{SupportedEngineVersion}' `
    -SkipUserEnvironmentRegistration
$env:UE_ADDITIONAL_PLUGIN_PATHS = 'D:\UnrealPlugins\{SupportedEngineVersion}'
& 'D:\EpicGame\UE_{SupportedEngineVersion}\Engine\Binaries\Win64\UnrealEditor.exe'
```

Unreal automatically selects **Light** or **Dark** to match Windows. To choose a theme manually, disable **Follow Windows Theme** under **Editor Preferences > Plugins > Light Theme Fix** and restart the editor.

The bundled theme is copied once to the current user's Unreal theme directory. Existing copies are intentionally preserved so local theme edits are never overwritten.

## Configuration

Open **Editor Preferences > Plugins > Light Theme Fix**. Settings marked as restart-required rebuild the parent Slate style at the next editor launch. Graph grid and execution-wire colors update with theme changes; tooltip and focused-input corrections run only while their feature switches are enabled.

`Follow Windows Theme` is enabled by default and reads Windows' `AppsUseLightTheme` preference at startup and at the configured check interval. Automatic switches are persisted to Unreal's application-appearance setting, so the preference page stays in sync.

`Apply Only to Light Themes` is enabled by default. It gates graph grid/wire colors and runtime tooltip/input corrections when Unreal's dark theme is active. Parent-style changes marked as restart-required are installed at startup, so disable their individual feature switches if you do not want those overrides.

## Building from source

For plugin development, place the source repository in `<Project>/Plugins/LightThemeFix` and build the project's Editor target. Do not point `UE_ADDITIONAL_PLUGIN_PATHS` at an unbuilt source checkout: Unreal's environment-based discovery is intended for precompiled plugins.

To produce the same distributable archive used for releases:

```powershell
.\Scripts\BuildRelease.ps1 -EngineRoot 'D:\EpicGame\UE_5.8'
```

The script runs Unreal Automation Tool's `BuildPlugin`, creates a versioned zip under `Dist`, and writes a matching SHA-256 file. Debug symbols are omitted by default; pass `-IncludeDebugSymbols` to retain PDB files.

## Troubleshooting

- **The theme is not listed:** verify the plugin loaded, then restart the editor. The installed file is named `Light.json` in Unreal's per-user theme directory.
- **Unreal reports incompatible binaries:** use a release built for the exact engine version, or delete the plugin's `Binaries` and `Intermediate` directories and compile the Editor target locally.
- **A search field still uses white focused text:** include the editor panel, engine version, and a screenshot in the issue. Some editor modules use concrete Slate subclasses rather than the base `SEditableTextBox` type.
- **A setting appears unchanged:** options with the restart marker are captured when the Slate parent style is created and require an editor restart.

## Compatibility and scope

- Tested: Unreal Engine 5.8, Win64
- Module type: Editor only; it is not loaded into packaged games
- No project assets or gameplay code are modified
- Third-party editor widgets with private styles may need an additional targeted override

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Visual fixes should be checked in both the bundled light theme and Unreal's default dark theme, and every release must pass an editor shutdown test.

## License

[MIT](LICENSE)

## 中文说明

发行包按 `UE_ADDITIONAL_PLUGIN_PATHS` 外部插件模式设计。解压对应 UE 版本的发行包、关闭 Unreal Editor，然后运行：

```powershell
.\LightThemeFix\Scripts\InstallExternalPlugin.ps1
```

默认安装目录为 `%LOCALAPPDATA%\UnrealEngine\{支持的引擎版本}\ExternalPlugins\LightThemeFix`。脚本会从发行包 `.uplugin` 中声明的 `EngineVersion` 自动提取主版本和次版本：例如 `5.8.0` 使用 `5.8` 目录，将来 `5.9.0` 会自动使用 `5.9` 目录。脚本会把对应版本的 `ExternalPlugins` 目录加入当前用户的 `UE_ADDITIONAL_PLUGIN_PATHS`。插件默认启用，因此不需要复制到每个工程，也不需要逐个工程启用。该环境变量会被所有 UE 版本读取；若需要严格隔离版本，请使用 `-SkipUserEnvironmentRegistration`，并仅在启动对应版本编辑器的进程中临时设置环境变量。

环境变量只在进程启动时继承。安装器会向 Windows 广播环境更新，供之后启动的桌面进程继承；但已经运行的 Epic Games Launcher 仍会保留旧环境。必须把启动器及其托盘进程完全退出后再打开 Unreal Editor；只从同一个启动器进程重新打开工程，仍会使用旧的插件搜索路径。

插件默认跟随 Windows 的应用主题：浅色模式自动使用 **Light**，深色模式自动使用 **Dark**，并会在编辑器运行期间响应系统主题变化。如需手动选择主题，请在“编辑器偏好设置 > 插件 > Light Theme Fix”中关闭 **Follow Windows Theme** 后重启编辑器。插件配置按“用户 + UE 版本”保存，同一 UE 版本打开的所有工程共享配置。

上方截图展示了启用 Light 主题与插件对比度修复后的编辑器效果。

所有功能开关与颜色参数位于“编辑器偏好设置 > 插件 > Light Theme Fix”。插件只修改编辑器界面，不会改动游戏资产或进入打包后的游戏。外部目录只支持预编译插件；若 DLL 与你的 UE 版本不一致，请下载匹配版本的发行包或重新运行发行构建脚本。

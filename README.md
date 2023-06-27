# 10Mica
[Mica](https://learn.microsoft.com/en-us/windows/apps/design/style/mica) but for Windows 10!

> **Warning**
> The code is still messy, incomplete, full of duplications, and not ready to be used in production yet

## Media
https://github.com/ahmed605/10Mica/assets/34550324/8b8bc16f-5399-4cc1-947b-77ab807bc6de

## Usage

> **Note**
> While the UWP example below set the brush on the `Page` element, it's **very** recommended to use it on the `Frame` element instead as you **can't** have more than one `TenMicaBrush` instance running, so using it on the `Frame` element assures that there's only one instance running. 

### UWP

```xml
<Page xmlns:tenmica="using:TenMica" ...>
  <Page.Background>
    <tenmica:TenMicaBrush EnabledInActivatedNotForeground="True" ForcedTheme="Dark"/>
  </Page.Background>
</Page>
```

### WinUI 3

```xml
<Window
    x:Class="TenMicaTestWasdk.MainWindow"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:local="using:TenMicaTestWasdk"
    xmlns:d="http://schemas.microsoft.com/expression/blend/2008"
    xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006"
    xmlns:mica="using:TenMica"
    mc:Ignorable="d">

    <Window.SystemBackdrop>
        <mica:TenMicaBackdrop EnabledInActivatedNotForeground="True"/>
    </Window.SystemBackdrop>

    ...
</Window>
```


> **Note**
> 10Mica is currently supports 15063+ on the UWP version and 18362+ on the WASDK version.

## Customers

### Oxidized

![Oxidized_but_something_is_different](https://github.com/ahmed605/10Mica/assets/34550324/95aca844-44eb-4c0f-b0e4-f2738a084053)


### [FlairMax](https://www.microsoft.com/store/apps/9PDZVJ34ZTXG)

![Screenshot_2023-05-12_174506](https://github.com/ahmed605/10Mica/assets/34550324/c9803ed6-cf81-4101-bec1-470ea2c0f906)

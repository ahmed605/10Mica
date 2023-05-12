# 10Mica
[Mica](https://learn.microsoft.com/en-us/windows/apps/design/style/mica) but for Windows 10!

> **Warning**
> The code is still messy, incomplete, full of duplications, and not ready to be used in production yet

## Media
https://github.com/ahmed605/10Mica/assets/34550324/22405269-ee5c-4cae-a4ef-320aeb5d2207

## Usage

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
    <Grid>
        <Grid.Background>
            <mica:TenMicaBrush TargetWindow="{x:Bind}" EnabledInActivatedNotForeground="True"/>
        </Grid.Background>
    </Grid>
</Window>
```

## Customers

### Oxidized

![Oxidized_but_something_is_different](https://github.com/ahmed605/10Mica/assets/34550324/751a8725-a389-43c0-8c61-1465ffa2a79a)


### [FlairMax](https://www.microsoft.com/store/apps/9PDZVJ34ZTXG)

![Screenshot_2023-05-12_174506](https://github.com/ahmed605/10Mica/assets/34550324/f49f9a55-d753-4fcb-8c5d-32ad3bd07a26)

# ppm2png.ps1 —— 把 P3 文本 PPM 转成 PNG，方便直接预览
# 用法: .\tools\ppm2png.ps1 -InputPath output\image.ppm -OutputPath output\image.png

param(
    [Parameter(Mandatory = $true)][string]$InputPath,
    [Parameter(Mandatory = $true)][string]$OutputPath
)

Add-Type -AssemblyName System.Drawing

$code = @"
using System;
using System.IO;
using System.Drawing;
using System.Text.RegularExpressions;

public class PpmConverter
{
    public static void ToPng(string inPath, string outPath)
    {
        if (!File.Exists(inPath))
            throw new FileNotFoundException("找不到输入文件: " + inPath);

        string text = File.ReadAllText(inPath);
        text = Regex.Replace(text, @"#[^\n]*", " ");   // 去掉 # 注释
        string[] t = text.Split(new char[] { ' ', '\t', '\r', '\n' },
                                StringSplitOptions.RemoveEmptyEntries);

        if (t[0] != "P3")
            throw new NotSupportedException("只支持 P3 文本格式，当前魔数: " + t[0]);

        int w    = int.Parse(t[1]);
        int h    = int.Parse(t[2]);
        int maxv = int.Parse(t[3]);
        double scale = 255.0 / maxv;

        using (Bitmap bmp = new Bitmap(w, h, System.Drawing.Imaging.PixelFormat.Format24bppRgb))
        {
            Rectangle rect = new Rectangle(0, 0, w, h);
            var data = bmp.LockBits(rect, System.Drawing.Imaging.ImageLockMode.WriteOnly, bmp.PixelFormat);
            int stride = data.Stride;
            byte[] buf = new byte[stride * h];
            int idx = 4;
            for (int y = 0; y < h; y++)
            {
                int row = y * stride;
                for (int x = 0; x < w; x++)
                {
                    byte r = (byte)(int.Parse(t[idx++]) * scale);
                    byte g = (byte)(int.Parse(t[idx++]) * scale);
                    byte b = (byte)(int.Parse(t[idx++]) * scale);
                    buf[row + x * 3 + 0] = b;   // BGR
                    buf[row + x * 3 + 1] = g;
                    buf[row + x * 3 + 2] = r;
                }
            }
            System.Runtime.InteropServices.Marshal.Copy(buf, 0, data.Scan0, buf.Length);
            bmp.UnlockBits(data);
            bmp.Save(outPath, System.Drawing.Imaging.ImageFormat.Png);
        }
    }
}
"@

Add-Type -TypeDefinition $code -ReferencedAssemblies System.Drawing

# 统一转成绝对路径：GDI+ 对相对路径容易报一般性错误
$fullIn  = (Resolve-Path $InputPath).Path
$fullOut = if ([System.IO.Path]::IsPathRooted($OutputPath)) {
    $OutputPath
} else {
    Join-Path (Get-Location).Path $OutputPath
}

[PpmConverter]::ToPng($fullIn, $fullOut)
Write-Host "Converted -> $fullOut"

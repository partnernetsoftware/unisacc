# handoff 2026-11-18 D2/D3/D4 视频

先前本机无这些 x.com 视频缓存(搜过 /tmp Downloads .cache; 未把 cursor-agent mp4 当它们)。无 ffmpeg、无 yt-dlp。无 cookie。

方法: GET https://api.fxtwitter.com/<user>/status/<id> -> tweet.media.videos[].url -> curl mp4。

| 项 | 视频路径 | sha256 | 帧 |
|---|---|---|---|
| D3 | /tmp/ref/d3.mp4 | 679a804c067ac109c6e155051be646776fee9912a8655760a85039f04698e2ea | ref/d3/frames 01 04 07 |
| D4 | /tmp/ref/d4.mp4 | 5052c61911f2c97ee281dd3bba6a32bcd4b0522599ed3a2fce62904291114371 | ref/d4/frames 01 04 07 10 13 |
| D2 | 无 mp4 | card /tmp/ref/d2-card.jpg b9758d9ffa46a4c88f3433918335bdb69cd589050307567a001e16741c8e79b1 | ref/d2/frames/01.jpg 仅 card |

抽帧: google-chrome --headless --screenshot; 非 0 秒多数 seek 失败(3021B 空白)。ffmpeg 缺口记 build/pkg-gaps.tsv。帧不进 docs/。

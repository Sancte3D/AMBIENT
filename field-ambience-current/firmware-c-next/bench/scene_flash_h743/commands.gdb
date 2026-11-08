# source this file after connecting to a deliberately flashed H743 bench image.
# Definitions only: sourcing never writes a mailbox or starts a flash operation.
define scene_bench_status
    print g_scene_bench
end
define scene_bench_publish
    set g_scene_bench.request_id = g_scene_bench.ack_id + 1
    if g_scene_bench.request_id == 0
        set g_scene_bench.request_id = 1
    end
end
define scene_bench_recover
    set g_scene_bench.command = 1
    set g_scene_bench.argument = 0
    set g_scene_bench.key = 0
    scene_bench_publish
end
define scene_bench_read
    if $argc != 1
        echo Usage: scene_bench_read <relative byte offset, aligned to32>\n
    else
        set g_scene_bench.command = 2
        set g_scene_bench.argument = $arg0
        set g_scene_bench.key = 0
        scene_bench_publish
    end
end
define scene_bench_arm
    set g_scene_bench.command = 3
    set g_scene_bench.argument = 0
    set g_scene_bench.key = 0x41524d21
    scene_bench_publish
end
define scene_bench_save
    if $argc != 1
        echo Usage: scene_bench_save <pattern seed>; requires acknowledged arm\n
    else
        if g_scene_bench.arm_token == 0
            echo No acknowledged arm. Resume ARM first, then halt and inspect status.\n
        else
            set g_scene_bench.command = 4
            set g_scene_bench.argument = $arg0
            set g_scene_bench.key = g_scene_bench.arm_token
            scene_bench_publish
        end
    end
end
define scene_bench_cancel
    set g_scene_bench.command = 5
    set g_scene_bench.argument = 0
    set g_scene_bench.key = 0
    scene_bench_publish
end
define scene_bench_disarm
    set g_scene_bench.command = 6
    set g_scene_bench.argument = 0
    set g_scene_bench.key = 0
    scene_bench_publish
end

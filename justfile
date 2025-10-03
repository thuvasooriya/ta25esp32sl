default_env := "master"

default:
    @just --list

# upload to specific environment
up env="m":
    #!/usr/bin/env bash
    case {{ env }} in
        m|master)
            echo "Building and uploading MASTER..."
            pio run -e master -t upload
            ;;
        s1|panel1|p1)
            echo "Building and uploading PANEL 1..."
            pio run -e panel1 -t upload
            ;;
        s2|panel2|p2)
            echo "Building and uploading PANEL 2..."
            pio run -e panel2 -t upload
            ;;
        s3|panel3|p3)
            echo "Building and uploading PANEL 3..."
            pio run -e panel3 -t upload
            ;;
        s4|panel4|p4)
            echo "Building and uploading PANEL 4..."
            pio run -e panel4 -t upload
            ;;
        *)
            echo "Unknown environment: {{ env }}"
            echo "Usage: just up [m|s1|s2|s3|s4]"
            exit 1
            ;;
    esac

# build without uploading
build env="m":
    #!/usr/bin/env bash
    case {{ env }} in
        m|master)
            pio run -e master
            ;;
        s1|panel1|p1)
            pio run -e panel1
            ;;
        s2|panel2|p2)
            pio run -e panel2
            ;;
        s3|panel3|p3)
            pio run -e panel3
            ;;
        s4|panel4|p4)
            pio run -e panel4
            ;;
        all)
            echo "Building all environments..."
            pio run
            ;;
        *)
            echo "Unknown environment: {{ env }}"
            exit 1
            ;;
    esac

# monitor serial output for specific environment
mon env="m":
    #!/usr/bin/env bash
    case {{ env }} in
        m|master)
            pio device monitor -e master
            ;;
        s1|panel1|p1)
            pio device monitor -e panel1
            ;;
        s2|panel2|p2)
            pio device monitor -e panel2
            ;;
        s3|panel3|p3)
            pio device monitor -e panel3
            ;;
        s4|panel4|p4)
            pio device monitor -e panel4
            ;;
        *)
            echo "Unknown environment: {{ env }}"
            echo "Usage: just mon [m|s1|s2|s3|s4]"
            exit 1
            ;;
    esac

# upload and immediately monitor
flash env="m":
    just up {{ env }}
    just mon {{ env }}

# clean build files
clean:
    pio run -t clean

# clean all environments
clean-all:
    rm -rf .pio/build

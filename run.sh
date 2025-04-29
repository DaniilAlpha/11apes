for terminal in "$TERMINAL" x-terminal-emulator mate-terminal gnome-terminal terminator xfce4-terminal urxvt rxvt termit Eterm aterm uxterm xterm roxterm termite lxterminal terminology st qterminal lilyterm tilix terminix konsole kitty guake tilda alacritty hyper wezterm rio; do
    if command -v "$terminal" > /dev/null 2>&1; then
        TERMINAL="$terminal"
        break
    fi
done

cat > tmux.conf <<- "EOF"
new-session -s pdp11 -d -n "PDP-11"

set -g mouse on

split-window -v
select-pane -t 0
split-window -h

select-pane -t 2
resize-pane -D 100
resize-pane -U 18
send-keys "clear && build/main/main $ARGS 2> stderr" C-m

select-pane -t 0
send-keys "clear && tail -f stderr" C-m

select-pane -t 1
send-keys "clear && tail -f tty" C-m
EOF

tmux kill-session -t pdp11
$TERMINAL -e tmux -f ./tmux.conf attach-session -t pdp11 -- &
sleep 0.5
rm tmux.conf
wait
tmux kill-session -t pdp11

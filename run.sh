for terminal in "$TERMINAL" x-terminal-emulator mate-terminal gnome-terminal terminator xfce4-terminal urxvt rxvt termit Eterm aterm uxterm xterm roxterm termite lxterminal terminology st qterminal lilyterm tilix terminix konsole kitty guake tilda alacritty hyper wezterm rio; do
    if command -v "$terminal" > /dev/null 2>&1; then
        TERMINAL="$terminal"
        break
    fi
done

> stderr
> tty

cat > tmux.conf <<- "EOF"
new-session -s pdp11 -d -n "PDP-11"
set-option -g history-limit 8192
set -g mouse on

select-pane -t 0
split-window -v

select-pane -t 1
resize-pane -y 18
send-keys "clear && build/main/main $ARGS; tmux kill-session -t pdp11" C-m

select-pane -t 0
send-keys "clear && tail -f -s0.033 tty" C-m

select-pane -t 1
EOF

tmux kill-session -t pdp11
sleep 0.5
$TERMINAL -e tmux -f ./tmux.conf attach-session -t pdp11 -- &
sleep 1
rm tmux.conf
wait
tmux kill-session -t pdp11

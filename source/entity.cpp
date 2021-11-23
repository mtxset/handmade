
inline
Move_spec default_move_spec() {
    Move_spec result;
    
    result.max_acceleration_vector = false;
    result.speed = 1.0f;
    result.drag = 0.0f;
    result.boost = 0.0f;
    
    return result;
}

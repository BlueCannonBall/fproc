/// Represent a process as packed into a message
pub struct ManagedProcess {
    pub id: u32,
    pub name: String,
    pub pid: u32,
    pub running: bool
}
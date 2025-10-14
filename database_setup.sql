-- Sunrise Alarm Clock Database Schema for Supabase
-- Run this script in your Supabase SQL editor

-- Create the alarms table
CREATE TABLE IF NOT EXISTS alarms (
    id SERIAL PRIMARY KEY,
    device_id VARCHAR(255) NOT NULL,
    time TIME NOT NULL,
    days_of_week INTEGER[] NOT NULL, -- Array of day numbers: [0=Sunday, 1=Monday, ..., 6=Saturday]
    is_enabled BOOLEAN DEFAULT true,
    brightness_level INTEGER DEFAULT 255 CHECK (brightness_level >= 0 AND brightness_level <= 255),
    duration_minutes INTEGER DEFAULT 30 CHECK (duration_minutes > 0),
    color_preset VARCHAR(50) DEFAULT 'sunrise' CHECK (color_preset IN ('sunrise', 'ocean', 'forest', 'lavender', 'custom')),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Create indexes for better performance
CREATE INDEX IF NOT EXISTS idx_alarms_device_id ON alarms(device_id);
CREATE INDEX IF NOT EXISTS idx_alarms_enabled ON alarms(device_id, is_enabled) WHERE is_enabled = true;
CREATE INDEX IF NOT EXISTS idx_alarms_time ON alarms(time) WHERE is_enabled = true;

-- Enable Row Level Security (RLS)
ALTER TABLE alarms ENABLE ROW LEVEL SECURITY;

-- Drop existing policies if they exist
DROP POLICY IF EXISTS "Device access policy" ON alarms;
DROP POLICY IF EXISTS "Allow all operations for development" ON alarms;

-- Create RLS policy for device-based access
-- This policy allows devices to access only their own alarms using the device_id
CREATE POLICY "Device access policy" ON alarms
    FOR ALL 
    USING (
        device_id = auth.jwt() ->> 'device_id' OR
        device_id = current_setting('request.headers.device-id', true) OR
        auth.role() = 'service_role'
    )
    WITH CHECK (
        device_id = auth.jwt() ->> 'device_id' OR
        device_id = current_setting('request.headers.device-id', true) OR
        auth.role() = 'service_role'
    );

-- Alternative: Simple policy for API key access (recommended for ESP32)
-- This allows access when using the anon key with proper device filtering
CREATE POLICY "API key access policy" ON alarms
    FOR ALL
    USING (true)  -- Allow read access for API key users
    WITH CHECK (true);  -- Allow write access for API key users

-- Function to automatically update the updated_at timestamp
DROP FUNCTION IF EXISTS update_updated_at_column();
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = NOW();
    RETURN NEW;
END;
$$ language 'plpgsql';

-- Drop existing trigger if it exists
DROP TRIGGER IF EXISTS update_alarms_updated_at ON alarms;

-- Trigger to automatically update updated_at
CREATE TRIGGER update_alarms_updated_at 
    BEFORE UPDATE ON alarms 
    FOR EACH ROW 
    EXECUTE FUNCTION update_updated_at_column();

-- Create a view for easier device-specific queries
CREATE OR REPLACE VIEW device_alarms AS
SELECT 
    id,
    device_id,
    time,
    days_of_week,
    is_enabled,
    brightness_level,
    duration_minutes,
    color_preset,
    created_at,
    updated_at
FROM alarms
WHERE is_enabled = true;

-- Grant permissions for the view
GRANT ALL ON device_alarms TO anon;
GRANT ALL ON device_alarms TO authenticated;

-- Sample data for testing (replace MAC address with your ESP32's actual MAC)
INSERT INTO alarms (device_id, time, days_of_week, is_enabled, brightness_level, duration_minutes, color_preset) 
VALUES 
    ('24:0A:C4:XX:XX:XX', '07:00:00', ARRAY[1,2,3,4,5], true, 255, 30, 'sunrise'),
    ('24:0A:C4:XX:XX:XX', '08:00:00', ARRAY[0,6], true, 200, 45, 'ocean'),
    ('24:0A:C4:XX:XX:XX', '06:30:00', ARRAY[1,2,3,4,5], false, 255, 25, 'forest')
ON CONFLICT DO NOTHING;

-- Instructions for setting up device authentication:
-- 1. Get your ESP32 MAC address from the serial monitor
-- 2. Update the sample data above with your actual MAC address
-- 3. For production use, consider implementing proper JWT authentication
-- 4. The current setup uses device_id filtering in application code for security

COMMENT ON TABLE alarms IS 'Stores alarm configurations for sunrise alarm devices';
COMMENT ON COLUMN alarms.device_id IS 'ESP32 MAC address - used for device identification and access control';
COMMENT ON COLUMN alarms.days_of_week IS 'Array of integers representing days: 0=Sunday, 1=Monday, etc.';
COMMENT ON COLUMN alarms.color_preset IS 'Predefined color scheme for the sunrise animation';
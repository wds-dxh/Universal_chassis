/**
 * @file 通用底盘控制系统配置文件
 * @description 集中管理系统配置，方便用户修改
 */

const config = {
    // 服务器配置
    server: {
        port: 3000,
        host: 'localhost'
    },
    
    // MQTT配置
    mqtt: {
        brokerUrl: 'mqtt://ctl_car.dxh-wds.top:1883',
        options: {
            username: 'emqx_u',
            password: 'public',
            clientId: 'universal-chassis-' + Math.random().toString(16).substring(2, 8)
        },
        topicPrefix: {
            control: 'CarControl_',  // 控制指令前缀
            status: 'CarStatus_'     // 状态信息前缀
        }
    },
    
    // 会话配置
    session: {
        secret: 'universal-chassis-secret',
        cookie: {
            maxAge: 600000, // 10分钟
            secure: false   // 生产环境应设为true (需要HTTPS)
        }
    },
    
    // 认证配置
    auth: {
        password: '88888888', // 默认密码
        deviceIdPattern: /^\d{3}$/ // 设备ID格式：3位数字
    },
    
    // 控制参数默认值
    defaults: {
        speed: 0.5,      // 默认速度 (m/s)
        omega: 0.1,      // 默认角速度 (rad/s)
        acceleration: 10.0, // 默认加速度
        speedStep: 0.1,   // 速度调整步长
        commandDebounceTime: 20  // 降低到20ms提高响应速度
    },
    
    // 视频流配置
    video: {
        enabled: true,
        url: 'http://ctl_car.dxh-wds.top:8080/stream', // 视频流URL
        fallbackImage: '/images/no-video.png' // 视频不可用时显示的图片
    }
};

module.exports = config; 